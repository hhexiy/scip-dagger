/**@file   nodepru_policy.c
 * @brief  node pruner which prunes node according a learned policy 
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
#include "nodepru_policy.h"
#include "nodepru_oracle.h"
#include "nodesel_oracle.h"
#include "feat.h"
#include "policy.h"
#include "struct_policy.h"
#include "scip/sol.h"
#include "scip/tree.h"
#include "scip/set.h"
#include "scip/clock.h"
#include "scip/struct_set.h"
#include "scip/struct_scip.h"

#define NODEPRU_NAME            "policy"
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
   char*              polfname;           /**< name of the solution file */
   SCIP_POLICY*       policy;
   SCIP_FEAT*         feat;
   int                nprunes;
};

void SCIPnodeprupolicyPrintStatistics(
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
         "  nodes pruned     : %10d\n", nodeprudata->nprunes);
   SCIPmessageFPrintInfo(scip->messagehdlr, file, 
         "  pruning time     : %10.2f\n", SCIPnodepruGetTime(nodepru));
}

/** copy method for node pruner plugins (called when SCIP copies plugins) */
static
SCIP_DECL_NODEPRUCOPY(nodepruCopyPolicy)
{  /*lint --e{715}*/
   assert(scip != NULL);
   SCIP_CALL( SCIPincludeNodepruPolicy(scip) );

   return SCIP_OKAY;
}

/** solving process initialization method of node pruner (called when branch and bound process is about to begin) */
static
SCIP_DECL_NODEPRUINIT(nodepruInitPolicy)
{
   SCIP_NODEPRUDATA* nodeprudata;
   assert(scip != NULL);
   assert(nodepru != NULL);

   nodeprudata = SCIPnodepruGetData(nodepru);
   assert(nodeprudata != NULL);

   /* read policy */
   SCIP_CALL( SCIPpolicyCreate(scip, &nodeprudata->policy) );
   assert(nodeprudata->polfname != NULL);
   SCIP_CALL( SCIPreadLIBSVMPolicy(scip, nodeprudata->polfname, &nodeprudata->policy) );
   assert(nodeprudata->policy->weights != NULL);
  
   /* create feat */
   nodeprudata->feat = NULL;
   SCIP_CALL( SCIPfeatCreate(scip, &nodeprudata->feat, SCIP_FEATNODEPRU_SIZE) );
   assert(nodeprudata->feat != NULL);
   SCIPfeatSetMaxDepth(nodeprudata->feat, SCIPgetNBinVars(scip) + SCIPgetNIntVars(scip));

   nodeprudata->nprunes = 0;
 
   return SCIP_OKAY;
}

/** destructor of node pruner to free user data (called when SCIP is exiting) */
static
SCIP_DECL_NODEPRUEXIT(nodepruExitPolicy)
{
   SCIP_NODEPRUDATA* nodeprudata;
   assert(scip != NULL);
   assert(nodepru != NULL);

   nodeprudata = SCIPnodepruGetData(nodepru);

   assert(nodeprudata->feat != NULL);
   SCIP_CALL( SCIPfeatFree(scip, &nodeprudata->feat) );

   assert(nodeprudata->policy != NULL);
   SCIP_CALL( SCIPpolicyFree(scip, &nodeprudata->policy) );
  
   return SCIP_OKAY;
}

/** destructor of node pruner to free user data (called when SCIP is exiting) */
static
SCIP_DECL_NODEPRUFREE(nodepruFreePolicy)
{
   SCIP_NODEPRUDATA* nodeprudata;
   nodeprudata = SCIPnodepruGetData(nodepru);

   assert(nodeprudata != NULL);

   SCIPfreeBlockMemory(scip, &nodeprudata);

   SCIPnodepruSetData(nodepru, NULL);

   return SCIP_OKAY;
}

/** node pruning method of node pruner */
static
SCIP_DECL_NODEPRUPRUNE(nodepruPrunePolicy)
{
   SCIP_NODEPRUDATA* nodeprudata;

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
      /*
      SCIPclockStart(nodeprudata->featcalctime, scip->set);
      SCIPclockStop(nodeprudata->featcalctime, scip->set);
      */
      SCIPcalcNodeScore(node, nodeprudata->feat, nodeprudata->policy);

      if( SCIPsetIsGT(scip->set, SCIPnodeGetScore(node), 0) )
      {
         *prune = TRUE;
         nodeprudata->nprunes++;
         SCIPdebugMessage("pruning node: #%"SCIP_LONGINT_FORMAT"\n", SCIPnodeGetNumber(node));
      }
      else
         *prune = FALSE;
   }

   return SCIP_OKAY;
}


/*
 * node pruner specific interface methods
 */

/** creates the uct node pruner and includes it in SCIP */
SCIP_RETCODE SCIPincludeNodepruPolicy(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_NODEPRUDATA* nodeprudata;
   SCIP_NODEPRU* nodepru;

   /* create policy node pruner data */
   SCIP_CALL( SCIPallocBlockMemory(scip, &nodeprudata) );

   nodepru = NULL;
   nodeprudata->polfname = NULL;

   /* use SCIPincludeNodepruBasic() plus setter functions if you want to set callbacks one-by-one and your code should
    * compile independent of new callbacks being added in future SCIP versions
    */
   SCIP_CALL( SCIPincludeNodepruBasic(scip, &nodepru, NODEPRU_NAME, NODEPRU_DESC, NODEPRU_STDPRIORITY,
          NODEPRU_MEMSAVEPRIORITY, nodepruPrunePolicy, nodeprudata) );

   assert(nodepru != NULL);

   /* set non fundamental callbacks via setter functions */
   SCIP_CALL( SCIPsetNodepruCopy(scip, nodepru, nodepruCopyPolicy) );
   SCIP_CALL( SCIPsetNodepruInit(scip, nodepru, nodepruInitPolicy) );
   SCIP_CALL( SCIPsetNodepruExit(scip, nodepru, nodepruExitPolicy) );
   SCIP_CALL( SCIPsetNodepruFree(scip, nodepru, nodepruFreePolicy) );

   /* add policy node pruner parameters */
   SCIP_CALL( SCIPaddStringParam(scip, 
         "nodepruning/"NODEPRU_NAME"/polfname",
         "name of the policy model file",
         &nodeprudata->polfname, FALSE, DEFAULT_FILENAME, NULL, NULL) );

   return SCIP_OKAY;
}
