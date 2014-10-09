/**@file   nodesel_dagger.c
 * @brief  uct node selector which balances exploration and exploitation by considering node visits
 * @author Gregor Hendel
 *
 * the UCT node selection rule selects the next leaf according to a mixed score of the node's actual lower bound
 *
 * The idea of UCT node selection for MIP appeared in:
 *
 * The authors adapted a game-tree exploration scheme called UCB to MIP trees. Starting from the root node as current node,
 *
 * The node selector features several parameters:
 *
 * @note It should be avoided to switch to uct node selection after the branch and bound process has begun because
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/
#define SCIP_DEBUG
#include <assert.h>
#include <string.h>
#include "nodesel_dagger.h"
#include "nodesel_oracle.h"
#include "feat.h"
#include "policy.h"
#include "struct_policy.h"
#include "scip/sol.h"
#include "scip/tree.h"
#include "scip/struct_set.h"
#include "scip/struct_scip.h"

#define NODESEL_NAME            "dagger"
#define NODESEL_DESC            "node selector which selects node according to a policy but writes exampels according to the oracle"
#define NODESEL_STDPRIORITY     10
#define NODESEL_MEMSAVEPRIORITY 0

#define DEFAULT_FILENAME        ""

/*
 * Data structures
 */

/** node selector data */
struct SCIP_NodeselData
{
   char*              solfname;           /**< name of the solution file */
   SCIP_SOL*          optsol;             /**< optimal solution */
   char*              polfname;           /**< name of the solution file */
   SCIP_POLICY*       policy;
   char*              trjfname;           /**< name of the trajectory file */
   FILE*              trjfile;
   SCIP_FEAT*         feat;
   SCIP_FEAT*         optfeat;
#ifndef NDEBUG
   SCIP_Longint       optnodenumber;      /**< successively assigned number of the node */
#endif
   SCIP_Bool          negate;
   int                nerrors;            /**< number of wrong ranking of a pair of nodes */
   int                ncomps;              /**< total number of comparisons */
};

void SCIPnodeseldaggerPrintStatistics(
   SCIP*                 scip,
   SCIP_NODESEL*         nodesel,
   FILE*                 file
   )
{
   SCIP_NODESELDATA* nodeseldata;

   assert(scip != NULL);
   assert(nodesel != NULL);

   nodeseldata = SCIPnodeselGetData(nodesel);
   assert(nodeseldata != NULL);

   SCIPmessageFPrintInfo(scip->messagehdlr, file, 
         "Node selector      :\n");
   SCIPmessageFPrintInfo(scip->messagehdlr, file, 
         "  comp error rate  : %d/%d\n", nodeseldata->nerrors, nodeseldata->ncomps);
   SCIPmessageFPrintInfo(scip->messagehdlr, file, 
         "  selection time   : %10.2f\n", SCIPnodeselGetTime(nodesel));
}

/** copy method for node selector plugins (called when SCIP copies plugins) */
static
SCIP_DECL_NODESELCOPY(nodeselCopyDagger)
{  /*lint --e{715}*/
   assert(scip != NULL);
   SCIP_CALL( SCIPincludeNodeselDagger(scip) );

   return SCIP_OKAY;
}

/** solving process initialization method of node selector (called when branch and bound process is about to begin) */
static
SCIP_DECL_NODESELINITSOL(nodeselInitsolDagger)
{
   SCIP_NODESELDATA* nodeseldata;
   assert(scip != NULL);
   assert(nodesel != NULL);

   nodeseldata = SCIPnodeselGetData(nodesel);
   assert(nodeseldata != NULL);

   /* solfname should be set before including nodeseldagger */
   assert(nodeseldata->solfname != NULL);
   nodeseldata->optsol = NULL;
   SCIP_CALL( SCIPreadOptSol(scip, nodeseldata->solfname, &nodeseldata->optsol) );
   assert(nodeseldata->optsol != NULL);
   SCIP_CALL( SCIPprintSol(scip, nodeseldata->optsol, NULL, FALSE) ); 

   /* read policy */
   SCIP_CALL( SCIPpolicyCreate(scip, &nodeseldata->policy) );
   assert(nodeseldata->polfname != NULL);
   SCIP_CALL( SCIPreadLIBSVMPolicy(scip, nodeseldata->polfname, &nodeseldata->policy) );
   assert(nodeseldata->policy->weights != NULL);
  
   /* open trajectory file for writing */
   /* open in appending mode for writing training file from multiple problems */
   nodeseldata->trjfile = NULL;
   if( nodeseldata->trjfname != NULL )
      nodeseldata->trjfile = fopen(nodeseldata->trjfname, "a");

   /* create feat */
   nodeseldata->feat = NULL;
   SCIP_CALL( SCIPfeatCreate(scip, &nodeseldata->feat, SCIP_FEATNODESEL_SIZE) );
   assert(nodeseldata->feat != NULL);
   SCIPfeatSetMaxDepth(nodeseldata->feat, SCIPgetNVars(scip));
  
   /* create optimal node feat */
   nodeseldata->optfeat = NULL;
   SCIP_CALL( SCIPfeatCreate(scip, &nodeseldata->optfeat, SCIP_FEATNODESEL_SIZE) );
   assert(nodeseldata->optfeat != NULL);
   SCIPfeatSetMaxDepth(nodeseldata->optfeat, SCIPgetNVars(scip));

#ifndef NDEBUG
   nodeseldata->optnodenumber = -1;
#endif
   nodeseldata->negate = TRUE;

   nodeseldata->nerrors = 0;
   nodeseldata->ncomps = 0;

   return SCIP_OKAY;
}

/** destructor of node selector to free user data (called when SCIP is exiting) */
static
SCIP_DECL_NODESELFREE(nodeselFreeDagger)
{
   SCIP_NODESELDATA* nodeseldata;
   assert(scip != NULL);
   assert(nodesel != NULL);

   nodeseldata = SCIPnodeselGetData(nodesel);

   assert(nodeseldata->optsol != NULL);
   SCIP_CALL( SCIPfreeSolSelf(scip, &nodeseldata->optsol) );
  
   if( nodeseldata->trjfile != NULL)
      fclose(nodeseldata->trjfile);

   assert(nodeseldata->feat != NULL);
   SCIP_CALL( SCIPfeatFree(scip, &nodeseldata->feat) );
   if( nodeseldata->optfeat != NULL )
      SCIP_CALL( SCIPfeatFree(scip, &nodeseldata->optfeat) );

   assert(nodeseldata->policy != NULL);
   SCIP_CALL( SCIPpolicyFree(scip, &nodeseldata->policy) );
   
   SCIPfreeBlockMemory(scip, &nodeseldata);

   SCIPnodeselSetData(nodesel, NULL);

   return SCIP_OKAY;
}

/** node selection method of node selector */
static
SCIP_DECL_NODESELSELECT(nodeselSelectDagger)
{
   SCIP_NODESELDATA* nodeseldata;
   SCIP_NODE** leaves;
   SCIP_NODE** children;
   SCIP_NODE** siblings;
   int nleaves;
   int nsiblings;
   int nchildren;
   int optchild;
   int i;

   assert(nodesel != NULL);
   assert(strcmp(SCIPnodeselGetName(nodesel), NODESEL_NAME) == 0);
   assert(scip != NULL);
   assert(selnode != NULL);

   nodeseldata = SCIPnodeselGetData(nodesel);
   assert(nodeseldata != NULL);

   /* collect leaves, children and siblings data */
   SCIP_CALL( SCIPgetOpenNodesData(scip, &leaves, &children, &siblings, &nleaves, &nchildren, &nsiblings) );

   /* check newly created nodes */
   optchild = -1;
   for( i = 0; i < nchildren; i++)
   {
      /* compute score */
      SCIPcalcNodeselFeat(scip, children[i], nodeseldata->feat);
      SCIPcalcNodeScore(children[i], nodeseldata->feat, nodeseldata->policy);

      /* check optimality */
      SCIP_CALL( SCIPnodeCheckOptimal(scip, children[i], nodeseldata->optsol) ); 
      SCIPnodeSetOptchecked(children[i]);
      if( SCIPnodeIsOptimal(children[i]) )
      {
#ifndef NDEBUG
         SCIPdebugMessage("opt node #%"SCIP_LONGINT_FORMAT"\n", SCIPnodeGetNumber(children[i]));
         nodeseldata->optnodenumber = SCIPnodeGetNumber(children[i]);
#endif
         optchild = i;
      }
   }

   /* write examples */
   if( nodeseldata->trjfile != NULL )
   {
      if( optchild != -1 )
      {
         /* new optimal node */
         SCIPcalcNodeselFeat(scip, children[optchild], nodeseldata->optfeat);
         for( i = 0; i < nchildren; i++)
         {
            if( i != optchild )
            {
               SCIPcalcNodeselFeat(scip, children[i], nodeseldata->feat);
               nodeseldata->negate ^= 1;
#ifndef NDEBUG
               SCIPdebugMessage("example  #%d #%d\n", (int)nodeseldata->optnodenumber, (int)SCIPnodeGetNumber(children[i]));
#endif
               SCIPfeatDiffLIBSVMPrint(scip, nodeseldata->trjfile, nodeseldata->optfeat, nodeseldata->feat, 1, nodeseldata->negate);
               SCIPfeatDiffLIBSVMPrint(scip, NULL, nodeseldata->optfeat, nodeseldata->feat, 1, nodeseldata->negate);
            }
         }
         for( i = 0; i < nsiblings; i++ )
         {
            SCIPcalcNodeselFeat(scip, siblings[i], nodeseldata->feat);
            nodeseldata->negate ^= 1;
#ifndef NDEBUG
            SCIPdebugMessage("example  #%d #%d\n", (int)nodeseldata->optnodenumber, (int)SCIPnodeGetNumber(siblings[i]));
#endif
            SCIPfeatDiffLIBSVMPrint(scip, nodeseldata->trjfile, nodeseldata->optfeat, nodeseldata->feat, 1, nodeseldata->negate);
            SCIPfeatDiffLIBSVMPrint(scip, NULL, nodeseldata->optfeat, nodeseldata->feat, 1, nodeseldata->negate);
         }
         for( i = 0; i < nleaves; i++ )
         {
            SCIPcalcNodeselFeat(scip, leaves[i], nodeseldata->feat);
            nodeseldata->negate ^= 1;
#ifndef NDEBUG
            SCIPdebugMessage("example  #%d #%d\n", (int)nodeseldata->optnodenumber, (int)SCIPnodeGetNumber(leaves[i]));
#endif
            SCIPfeatDiffLIBSVMPrint(scip, nodeseldata->trjfile, nodeseldata->optfeat, nodeseldata->feat, 1, nodeseldata->negate);
            SCIPfeatDiffLIBSVMPrint(scip, NULL, nodeseldata->optfeat, nodeseldata->feat, 1, nodeseldata->negate);
         }
      }
      else
      {
         /* children are not optimal */
         assert(nchildren == 0 || (nchildren > 0 && nodeseldata->optnodenumber != -1));
         for( i = 0; i < nchildren; i++ )
         {
            SCIPcalcNodeselFeat(scip, children[i], nodeseldata->feat);
            nodeseldata->negate ^= 1;
#ifndef NDEBUG
            SCIPdebugMessage("example  #%d #%d\n", (int)nodeseldata->optnodenumber, (int)SCIPnodeGetNumber(children[i]));
#endif
            SCIPfeatDiffLIBSVMPrint(scip, nodeseldata->trjfile, nodeseldata->optfeat, nodeseldata->feat, 1, nodeseldata->negate);
            SCIPfeatDiffLIBSVMPrint(scip, NULL, nodeseldata->optfeat, nodeseldata->feat, 1, nodeseldata->negate);
         }
      }
   }

   *selnode = SCIPgetBestNode(scip);

   return SCIP_OKAY;
}

/** node comparison method of dagger node selector */
static
SCIP_DECL_NODESELCOMP(nodeselCompDagger)
{  /*lint --e{715}*/
   SCIP_Real score1;
   SCIP_Real score2;
   SCIP_Bool isopt1;
   SCIP_Bool isopt2;
   SCIP_NODESELDATA* nodeseldata;
   int result;

   assert(nodesel != NULL);
   assert(strcmp(SCIPnodeselGetName(nodesel), NODESEL_NAME) == 0);
   assert(scip != NULL);

   assert(SCIPnodeIsOptchecked(node1) == TRUE);
   assert(SCIPnodeIsOptchecked(node2) == TRUE);

   score1 = SCIPnodeGetScore(node1);
   score2 = SCIPnodeGetScore(node2);

   assert(score1 != 0);
   assert(score2 != 0);

   if( SCIPisGT(scip, score1, score2) )
      result = -1;
   else if( SCIPisLT(scip, score1, score2) )
      result = +1;
   else
   {
      int depth1;
      int depth2;

      depth1 = SCIPnodeGetDepth(node1);
      depth2 = SCIPnodeGetDepth(node2);
      if( depth1 > depth2 )
         result = -1;
      else if( depth1 < depth2 )
         result = +1;
      else
      {
         SCIP_Real lowerbound1;
         SCIP_Real lowerbound2;

         lowerbound1 = SCIPnodeGetLowerbound(node1);
         lowerbound2 = SCIPnodeGetLowerbound(node2);
         if( SCIPisLT(scip, lowerbound1, lowerbound2) )
            result = -1;
         else if( SCIPisGT(scip, lowerbound1, lowerbound2) )
            result = +1;
         else
            result = 0;
      }
   }

   nodeseldata = SCIPnodeselGetData(nodesel);
   assert(nodeseldata != NULL);
   isopt1 = SCIPnodeIsOptimal(node1);
   isopt2 = SCIPnodeIsOptimal(node2);
   if( (isopt1 && result == 1) || (isopt2 && result == -1) )
      nodeseldata->nerrors++;
   nodeseldata->ncomps++;

   return result;
}

/*
 * node selector specific interface methods
 */

/** creates the uct node selector and includes it in SCIP */
SCIP_RETCODE SCIPincludeNodeselDagger(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_NODESELDATA* nodeseldata;
   SCIP_NODESEL* nodesel;

   /* create dagger node selector data */
   SCIP_CALL( SCIPallocBlockMemory(scip, &nodeseldata) );

   nodesel = NULL;
   nodeseldata->optsol = NULL;
   nodeseldata->solfname = NULL;
   nodeseldata->trjfname = NULL;
   nodeseldata->polfname = NULL;

   /* use SCIPincludeNodeselBasic() plus setter functions if you want to set callbacks one-by-one and your code should
    * compile independent of new callbacks being added in future SCIP versions
    */
   SCIP_CALL( SCIPincludeNodeselBasic(scip, &nodesel, NODESEL_NAME, NODESEL_DESC, NODESEL_STDPRIORITY,
          NODESEL_MEMSAVEPRIORITY, nodeselSelectDagger, nodeselCompDagger, nodeseldata) );

   assert(nodesel != NULL);

   /* set non fundamental callbacks via setter functions */
   SCIP_CALL( SCIPsetNodeselCopy(scip, nodesel, nodeselCopyDagger) );
   SCIP_CALL( SCIPsetNodeselInitsol(scip, nodesel, nodeselInitsolDagger) );
   SCIP_CALL( SCIPsetNodeselFree(scip, nodesel, nodeselFreeDagger) );

   /* add dagger node selector parameters */
   SCIP_CALL( SCIPaddStringParam(scip, 
         "nodeselection/"NODESEL_NAME"/solfname",
         "name of the optimal solution file",
         &nodeseldata->solfname, FALSE, DEFAULT_FILENAME, NULL, NULL) );
   SCIP_CALL( SCIPaddStringParam(scip, 
         "nodeselection/"NODESEL_NAME"/trjfname",
         "name of the file to write node selection trajectories",
         &nodeseldata->trjfname, FALSE, DEFAULT_FILENAME, NULL, NULL) );
   SCIP_CALL( SCIPaddStringParam(scip, 
         "nodeselection/"NODESEL_NAME"/polfname",
         "name of the policy model file",
         &nodeseldata->polfname, FALSE, DEFAULT_FILENAME, NULL, NULL) );

   return SCIP_OKAY;
}
