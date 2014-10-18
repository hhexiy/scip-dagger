/**@file   nodepru_dagger.c
 * @brief  node pruner which prunes node according to a learned policy and writes examples according to the oracle choice 
 * @author He He 
 *
 * the UCT node pruning rule pruects the next leaf according to a mixed score of the node's actual lower bound
 *
 * The idea of UCT node pruning for MIP appeared in:
 *
 * The authors adapted a game-tree exploration scheme called UCB to MIP trees. Starting from the root node as current node,
 *
 * The node pruner features several parameters:
 *
 * @note It should be avoided to switch to uct node pruning after the branch and bound process has begun because
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/
#include <assert.h>
#include <string.h>
#include "nodepru_dagger.h"
#include "nodepru_oracle.h"
#include "nodesel_oracle.h"
#include "feat.h"
#include "policy.h"
#include "struct_policy.h"
#include "scip/sol.h"
#include "scip/tree.h"
#include "scip/set.h"
#include "scip/struct_set.h"
#include "scip/struct_scip.h"

#define NODEPRU_NAME            "dagger"
#define NODEPRU_DESC            "node pruner which pruects node according to a policy but writes exampels according to the oracle"
#define NODEPRU_STDPRIORITY     10
#define NODEPRU_MEMSAVEPRIORITY 0

#define DEFAULT_FILENAME        ""

/*
 * Data structures
 */

/** node pruner data */
struct SCIP_NodepruData
{
   char*              solfname;           /**< name of the solution file */
   SCIP_SOL*          optsol;             /**< optimal solution */
   char*              polfname;           /**< name of the solution file */
   SCIP_POLICY*       policy;
   char*              trjfname;           /**< name of the trajectory file */
   FILE*              wfile;
   FILE*              trjfile;
   SCIP_FEAT*         feat;
   SCIP_Bool          checkopt;           /**< need to check node optimality? (don't need to if node selector is oracle or dagger */ 
   int                nprunes;            /**< number of nodes pruned */
   int                nnodes;             /**< number of nodes checked */
   int                nfalsepos;           /**< number of optimal nodes pruned */
   int                nfalseneg;           /**< number of non-optimal nodes not pruned */

};

void SCIPnodeprudaggerPrintStatistics(
   SCIP*                 scip,
   SCIP_NODEPRU*         nodepru,
   FILE*                 file
   )
{
   SCIP_NODEPRUDATA* nodeprudata;

   assert(scip != NULL);
   assert(nodepru != NULL);

   nodeprudata = SCIPnodepruGetData(nodepru);
   assert(nodeprudata != NULL);

   SCIPmessageFPrintInfo(scip->messagehdlr, file, 
         "Node pruner        :\n");
   SCIPmessageFPrintInfo(scip->messagehdlr, file, 
         "  nodes pruned     : %d/%d\n", nodeprudata->nprunes, nodeprudata->nnodes);
   SCIPmessageFPrintInfo(scip->messagehdlr, file, 
         "  FP pruned        : %d/%d\n", nodeprudata->nfalsepos, nodeprudata->nnodes);
   SCIPmessageFPrintInfo(scip->messagehdlr, file, 
         "  FN pruned        : %d/%d\n", nodeprudata->nfalseneg, nodeprudata->nnodes);
   SCIPmessageFPrintInfo(scip->messagehdlr, file, 
         "  pruning time     : %10.2f\n", SCIPnodepruGetTime(nodepru));
}

/** copy method for node pruner plugins (called when SCIP copies plugins) */
static
SCIP_DECL_NODEPRUCOPY(nodepruCopyDagger)
{  /*lint --e{715}*/
   assert(scip != NULL);
   SCIP_CALL( SCIPincludeNodepruDagger(scip) );

   return SCIP_OKAY;
}

/** solving process initialization method of node pruner (called when branch and bound process is about to begin) */
static
SCIP_DECL_NODEPRUINITSOL(nodepruInitsolDagger)
{
   SCIP_NODEPRUDATA* nodeprudata;
   assert(scip != NULL);
   assert(nodepru != NULL);

   nodeprudata = SCIPnodepruGetData(nodepru);
   assert(nodeprudata != NULL);

   /* solfname should be set before including nodeprudagger */
   assert(nodeprudata->solfname != NULL);
   nodeprudata->optsol = NULL;
   SCIP_CALL( SCIPreadOptSol(scip, nodeprudata->solfname, &nodeprudata->optsol) );
   assert(nodeprudata->optsol != NULL);
#ifdef SCIP_DEBUG
   SCIP_CALL( SCIPprintSol(scip, nodeprudata->optsol, NULL, FALSE) ); 
#endif

   /* read policy */
   SCIP_CALL( SCIPpolicyCreate(scip, &nodeprudata->policy) );
   assert(nodeprudata->polfname != NULL);
   SCIP_CALL( SCIPreadLIBSVMPolicy(scip, nodeprudata->polfname, &nodeprudata->policy) );
   assert(nodeprudata->policy->weights != NULL);
  
   /* open trajectory file for writing */
   /* open in appending mode for writing training file from multiple problems */
   nodeprudata->trjfile = NULL;
   if( nodeprudata->trjfname != NULL )
   {
      char wfname[100];
      strcpy(wfname, nodeprudata->trjfname);
      strcat(wfname, ".weight");
      nodeprudata->wfile = fopen(wfname, "a");
      nodeprudata->trjfile = fopen(nodeprudata->trjfname, "a");
   }

   /* create feat */
   nodeprudata->feat = NULL;
   SCIP_CALL( SCIPfeatCreate(scip, &nodeprudata->feat, SCIP_FEATNODEPRU_SIZE) );
   assert(nodeprudata->feat != NULL);
   SCIPfeatSetMaxDepth(nodeprudata->feat, SCIPgetNBinVars(scip) + SCIPgetNIntVars(scip));
  
   if( strcmp(SCIPnodeselGetName(SCIPgetNodesel(scip)), "oracle") == 0 ||
       strcmp(SCIPnodeselGetName(SCIPgetNodesel(scip)), "dagger") == 0 )
      nodeprudata->checkopt = FALSE;
   else
      nodeprudata->checkopt = TRUE;

   nodeprudata->nprunes = 0;
   nodeprudata->nnodes = 0;
   nodeprudata->nfalsepos = 0;
   nodeprudata->nfalseneg = 0;

   return SCIP_OKAY;
}

/** destructor of node pruner to free user data (called when SCIP is exiting) */
static
SCIP_DECL_NODEPRUFREE(nodepruFreeDagger)
{
   SCIP_NODEPRUDATA* nodeprudata;
   assert(scip != NULL);
   assert(nodepru != NULL);

   nodeprudata = SCIPnodepruGetData(nodepru);

   assert(nodeprudata->optsol != NULL);
   SCIP_CALL( SCIPfreeSolSelf(scip, &nodeprudata->optsol) );
  
   if( nodeprudata->trjfile != NULL)
   {
      fclose(nodeprudata->wfile);
      fclose(nodeprudata->trjfile);
   }

   assert(nodeprudata->feat != NULL);
   SCIP_CALL( SCIPfeatFree(scip, &nodeprudata->feat) );

   assert(nodeprudata->policy != NULL);
   SCIP_CALL( SCIPpolicyFree(scip, &nodeprudata->policy) );
   
   SCIPfreeBlockMemory(scip, &nodeprudata);

   SCIPnodepruSetData(nodepru, NULL);

   return SCIP_OKAY;
}

/** node pruning method of node pruner */
static
SCIP_DECL_NODEPRUPRUNE(nodepruPruneDagger)
{
   SCIP_NODEPRUDATA* nodeprudata;
   SCIP_Bool isoptimal;

   assert(nodepru != NULL);
   assert(strcmp(SCIPnodepruGetName(nodepru), NODEPRU_NAME) == 0);
   assert(scip != NULL);
   assert(node != NULL);

   nodeprudata = SCIPnodepruGetData(nodepru);
   assert(nodeprudata != NULL);

   /* don't prune the root */
   if( SCIPnodeGetDepth(node) == 0 )
   {
      *prune = FALSE;
      return SCIP_OKAY;
   }
   else
   {
      SCIPcalcNodepruFeat(scip, node, nodeprudata->feat);
      SCIPcalcNodeScore(node, nodeprudata->feat, nodeprudata->policy);

      if( SCIPsetIsGT(scip->set, SCIPnodeGetScore(node), 0) )
      {
         *prune = TRUE;
         SCIPdebugMessage("pruning node: #%"SCIP_LONGINT_FORMAT"\n", SCIPnodeGetNumber(node));
         nodeprudata->nprunes++;
      }
      else
         *prune = FALSE;
      nodeprudata->nnodes++;

#ifndef SCIP_DEBUG
      /* write examples */
      if( nodeprudata->trjfile != NULL )
      {
#endif
         if( nodeprudata->checkopt )
            SCIPnodeCheckOptimal(scip, node, nodeprudata->optsol);
         isoptimal = SCIPnodeIsOptimal(node);
         SCIPdebugMessage("node pruning feature of node #%"SCIP_LONGINT_FORMAT"\n", SCIPnodeGetNumber(node));
         SCIPfeatLIBSVMPrint(scip, nodeprudata->trjfile, nodeprudata->wfile, nodeprudata->feat, isoptimal ? -1 : 1);
         if( isoptimal && *prune )
            nodeprudata->nfalsepos++;
         else if( (!isoptimal) && (!*prune) )
            nodeprudata->nfalseneg++;
#ifndef SCIP_DEBUG
      }
#endif
   }

   return SCIP_OKAY;
}


/*
 * node pruner specific interface methods
 */

/** creates the uct node pruner and includes it in SCIP */
SCIP_RETCODE SCIPincludeNodepruDagger(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_NODEPRUDATA* nodeprudata;
   SCIP_NODEPRU* nodepru;

   /* create dagger node pruner data */
   SCIP_CALL( SCIPallocBlockMemory(scip, &nodeprudata) );

   nodepru = NULL;
   nodeprudata->optsol = NULL;
   nodeprudata->solfname = NULL;
   nodeprudata->trjfname = NULL;
   nodeprudata->polfname = NULL;

   /* use SCIPincludeNodepruBasic() plus setter functions if you want to set callbacks one-by-one and your code should
    * compile independent of new callbacks being added in future SCIP versions
    */
   SCIP_CALL( SCIPincludeNodepruBasic(scip, &nodepru, NODEPRU_NAME, NODEPRU_DESC, NODEPRU_STDPRIORITY,
          NODEPRU_MEMSAVEPRIORITY, nodepruPruneDagger, nodeprudata) );

   assert(nodepru != NULL);

   /* set non fundamental callbacks via setter functions */
   SCIP_CALL( SCIPsetNodepruCopy(scip, nodepru, nodepruCopyDagger) );
   SCIP_CALL( SCIPsetNodepruInitsol(scip, nodepru, nodepruInitsolDagger) );
   SCIP_CALL( SCIPsetNodepruFree(scip, nodepru, nodepruFreeDagger) );

   /* add dagger node pruner parameters */
   SCIP_CALL( SCIPaddStringParam(scip, 
         "nodepruning/"NODEPRU_NAME"/solfname",
         "name of the optimal solution file",
         &nodeprudata->solfname, FALSE, DEFAULT_FILENAME, NULL, NULL) );
   SCIP_CALL( SCIPaddStringParam(scip, 
         "nodepruning/"NODEPRU_NAME"/trjfname",
         "name of the file to write node pruning trajectories",
         &nodeprudata->trjfname, FALSE, DEFAULT_FILENAME, NULL, NULL) );
   SCIP_CALL( SCIPaddStringParam(scip, 
         "nodepruning/"NODEPRU_NAME"/polfname",
         "name of the policy model file",
         &nodeprudata->polfname, FALSE, DEFAULT_FILENAME, NULL, NULL) );

   return SCIP_OKAY;
}
