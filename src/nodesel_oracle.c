/**@file   nodesel_uct.c
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
#include "nodesel_oracle.h"
#include "feat.h"
#include "scip/sol.h"
#include "scip/tree.h"
#include "scip/struct_set.h"
#include "scip/struct_scip.h"

#define NODESEL_NAME            "oracle"
#define NODESEL_DESC            "node selector which always selects the optimal node"
#define NODESEL_STDPRIORITY     10
#define NODESEL_MEMSAVEPRIORITY 0

#define DEFAULT_FILENAME        ""

/*
 * Data structures
 */

/** node selector data */
struct SCIP_NodeselData
{
   SCIP_SOL*          optsol;             /**< optimal solution */
   char*              solfname;           /**< name of the solution file */
   char*              trjfname;           /**< name of the trajectory file */
   FILE*              trjfile;
   SCIP_FEAT*         feat;
   SCIP_FEAT*         optfeat;
   SCIP_Longint       optnodenumber;             /**< successively assigned number of the node */
   SCIP_Bool          negate;
};


/*
 * Local methods
 */

/** check if the given node include the optimal solution */
/* TODO: remove to ischecked */
SCIP_RETCODE SCIPnodeCheckOptimal(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_NODE*            node,               /**< the node in question */
   SCIP_SOL*             optsol              /**< node selector data */
   )
{
   /* get parent branch vars lead to this node */

   SCIP_VAR**            branchvars;         /* array of variables on which the branchings has been performed in all ancestors */
   SCIP_Real*            branchbounds;       /* array of bounds which the branchings in all ancestors set */
   SCIP_BOUNDTYPE*       boundtypes;         /* array of boundtypes which the branchings in all ancestors set */
   int                   nbranchvars;        /* number of variables on which branchings have been performed in all ancestors
                                              *   if this is larger than the array size, arrays should be reallocated and method should be called again */
   int                   branchvarssize;     /* available slots in arrays */

   int i;
   SCIP_NODE* parent;
   SCIP_Bool isoptimal = TRUE;


   assert(optsol != NULL);
   assert(node != NULL);
   assert(SCIPnodeIsOptchecked(node) == FALSE);

   /* don't consider root node */
   assert(SCIPnodeGetDepth(node) != 0);

   SCIPdebugMessage("checking node %d\n", (int)SCIPnodeGetNumber(node));

   /* check parent: if parent is not optimal, its subtree is not optimal */
   parent = SCIPnodeGetParent(node);
   /* root is always optimal */
   if( SCIPnodeGetDepth(parent) > 0 && !SCIPnodeIsOptimal(parent) )
      return SCIP_OKAY;

   branchvarssize = 1; 

   /* memory allocation */
   SCIP_CALL( SCIPallocBufferArray(scip, &branchvars, branchvarssize) );
   SCIP_CALL( SCIPallocBufferArray(scip, &branchbounds, branchvarssize) );
   SCIP_CALL( SCIPallocBufferArray(scip, &boundtypes, branchvarssize) );

   SCIPnodeGetParentBranchings(node, branchvars, branchbounds, boundtypes, &nbranchvars, branchvarssize);

   /* if the arrays were to small, we have to reallocate them and recall SCIPnodeGetParentBranchings */
   if( nbranchvars > branchvarssize )
   {
      branchvarssize = nbranchvars;

      /* memory reallocation */
      SCIP_CALL( SCIPreallocBufferArray(scip, &branchvars, branchvarssize) );
      SCIP_CALL( SCIPreallocBufferArray(scip, &branchbounds, branchvarssize) );
      SCIP_CALL( SCIPreallocBufferArray(scip, &boundtypes, branchvarssize) );
      
      SCIPnodeGetParentBranchings(node, branchvars, branchbounds, boundtypes, &nbranchvars, branchvarssize);
      assert(nbranchvars == branchvarssize);
   }

   /* check optimality */
   assert(nbranchvars >= 1);
   for( i = 0; i < nbranchvars; ++i)
   {
      SCIP_Real optval = SCIPgetSolVal(scip, optsol, branchvars[i]);
      if( (boundtypes[i] == SCIP_BOUNDTYPE_LOWER && optval < branchbounds[i]) ||
          (boundtypes[i] == SCIP_BOUNDTYPE_UPPER && optval > branchbounds[i]) )
      {
         isoptimal = FALSE;
         break;
      }
   }

   /* free all local memory */
   SCIPfreeBufferArray(scip, &branchvars);
   SCIPfreeBufferArray(scip, &boundtypes);
   SCIPfreeBufferArray(scip, &branchbounds);
  
   if( isoptimal )
      SCIPnodeSetOptimal(node);

   return SCIP_OKAY;
}

/** read the optimal solution (modified from readSol in reader_sol.c -- don't connect the solution with primal solutions) */
/** TODO: currently the read objective is wrong, since it doesn not include objetives from multi-aggregated variables */
SCIP_RETCODE SCIPreadOptSol(
   SCIP*                 scip,               /**< SCIP data structure */
   const char*           fname,              /**< name of the input file */
   SCIP_SOL**            sol                 /**< pointer to store the solution */
   )
{
   SCIP_FILE* file;
   SCIP_Bool error;
   SCIP_Bool unknownvariablemessage;
   SCIP_Bool usevartable;
   int lineno;

   assert(scip != NULL);
   assert(fname != NULL);

   SCIP_CALL( SCIPgetBoolParam(scip, "misc/usevartable", &usevartable) );

   if( !usevartable )
   {
      SCIPerrorMessage("Cannot read solution file if vartable is disabled. Make sure parameter 'misc/usevartable' is set to TRUE.\n");
      return SCIP_READERROR;
   }

   /* open input file */
   file = SCIPfopen(fname, "r");
   if( file == NULL )
   {
      SCIPerrorMessage("cannot open file <%s> for reading\n", fname);
      SCIPprintSysError(fname);
      return SCIP_NOFILE;
   }

   /* create zero solution */
   SCIP_CALL( SCIPcreateSolSelf(scip, sol, NULL) );
   assert(SCIPsolIsOriginal(*sol) == TRUE);
   
   /* read the file */
   error = FALSE;
   unknownvariablemessage = FALSE;
   lineno = 0;
   while( !SCIPfeof(file) && !error )
   {
      char buffer[SCIP_MAXSTRLEN];
      char varname[SCIP_MAXSTRLEN];
      char valuestring[SCIP_MAXSTRLEN];
      char objstring[SCIP_MAXSTRLEN];
      SCIP_VAR* var;
      SCIP_Real value;
      int nread;

      /* get next line */
      if( SCIPfgets(buffer, (int) sizeof(buffer), file) == NULL )
         break;
      lineno++;

      /* there are some lines which may preceed the solution information */
      if( strncasecmp(buffer, "solution status:", 16) == 0 || strncasecmp(buffer, "objective value:", 16) == 0 ||
         strncasecmp(buffer, "Log started", 11) == 0 || strncasecmp(buffer, "Variable Name", 13) == 0 ||
         strncasecmp(buffer, "All other variables", 19) == 0 || strncasecmp(buffer, "\n", 1) == 0 || 
         strncasecmp(buffer, "NAME", 4) == 0 || strncasecmp(buffer, "ENDATA", 6) == 0 )    /* allow parsing of SOL-format on the MIPLIB 2003 pages */
         continue;

      /* parse the line */
      nread = sscanf(buffer, "%s %s %s\n", varname, valuestring, objstring);
      if( nread < 2 )
      {
         SCIPerrorMessage("Invalid input line %d in solution file <%s>: <%s>.\n", lineno, fname, buffer);
         error = TRUE;
         break;
      }

      /* find the variable */
      var = SCIPfindVar(scip, varname);
      if( var == NULL )
      {
         if( !unknownvariablemessage )
         {
            SCIPverbMessage(scip, SCIP_VERBLEVEL_NORMAL, NULL, "unknown variable <%s> in line %d of solution file <%s>\n", 
               varname, lineno, fname);
            SCIPverbMessage(scip, SCIP_VERBLEVEL_NORMAL, NULL, "  (further unknown variables are ignored)\n");
            unknownvariablemessage = TRUE;
         }
         continue;
      }

      /* cast the value */
      if( strncasecmp(valuestring, "inv", 3) == 0 )
         continue;
      else if( strncasecmp(valuestring, "+inf", 4) == 0 || strncasecmp(valuestring, "inf", 3) == 0 )
         value = SCIPinfinity(scip);
      else if( strncasecmp(valuestring, "-inf", 4) == 0 )
         value = -SCIPinfinity(scip);
      else
      {
         nread = sscanf(valuestring, "%lf", &value);
         if( nread != 1 )
         {
            SCIPerrorMessage("Invalid solution value <%s> for variable <%s> in line %d of solution file <%s>.\n",
               valuestring, varname, lineno, fname);
            error = TRUE;
            break;
         }
      }

      /* set the solution value of the variable, if not multiaggregated */
      if( SCIPisTransformed(scip) && SCIPvarGetStatus(SCIPvarGetProbvar(var)) == SCIP_VARSTATUS_MULTAGGR )
      {
         SCIPverbMessage(scip, SCIP_VERBLEVEL_NORMAL, NULL, "ignored solution value for multiaggregated variable <%s>\n", SCIPvarGetName(var));
      }
      else
      {
         SCIP_RETCODE retcode;
         retcode =  SCIPsetSolVal(scip, *sol, var, value);

         if( retcode == SCIP_INVALIDDATA )
         {
            if( SCIPvarGetStatus(SCIPvarGetProbvar(var)) == SCIP_VARSTATUS_FIXED )
            {
               SCIPverbMessage(scip, SCIP_VERBLEVEL_NORMAL, NULL, "ignored conflicting solution value for fixed variable <%s>\n",
                  SCIPvarGetName(var));
            }
            else
            {
               SCIPverbMessage(scip, SCIP_VERBLEVEL_NORMAL, NULL, "ignored solution value for multiaggregated variable <%s>\n",
                  SCIPvarGetName(var));
            }
         }
         else
         {
            SCIP_CALL( retcode );
         }
      }
   }

   /* close input file */
   SCIPfclose(file);

   if( !error )
   {
      SCIP_Real obj;
      assert(SCIPgetNSols(scip) == 0);
      /* display result */
      obj = SCIPgetSolOrigObj(scip, *sol);
      SCIPverbMessage(scip, SCIP_VERBLEVEL_NORMAL, NULL, "optimal solution from solution file <%s> was %s\n",
         fname, "read");
      SCIPverbMessage(scip, SCIP_VERBLEVEL_NORMAL, NULL, "objective: %f\n", obj);
      return SCIP_OKAY;
   }
   else
   {
      /* free solution */
      SCIP_CALL( SCIPfreeSolSelf(scip, sol) );

      return SCIP_READERROR;
   }
}

/*
 * Callback methods of node selector
 */

/** copy method for node selector plugins (called when SCIP copies plugins) */
static
SCIP_DECL_NODESELCOPY(nodeselCopyOracle)
{  /*lint --e{715}*/
   assert(scip != NULL);
   SCIP_CALL( SCIPincludeNodeselOracle(scip) );

   return SCIP_OKAY;
}

/** solving process initialization method of node selector (called when branch and bound process is about to begin) */
static
SCIP_DECL_NODESELINITSOL(nodeselInitsolOracle)
{
   SCIP_NODESELDATA* nodeseldata;

   assert(scip != NULL);
   assert(nodesel != NULL);

   nodeseldata = SCIPnodeselGetData(nodesel);
   assert(nodeseldata != NULL);

   /** solfname should be set before including nodeseloracle */
   assert(nodeseldata->solfname != NULL);
   nodeseldata->optsol = NULL;
   SCIP_CALL( SCIPreadOptSol(scip, nodeseldata->solfname, &nodeseldata->optsol) );
   assert(nodeseldata->optsol != NULL);
   SCIP_CALL( SCIPprintSol(scip, nodeseldata->optsol, NULL, FALSE) ); 
   
   nodeseldata->trjfile = NULL;
   if( nodeseldata->trjfname != NULL )
      nodeseldata->trjfile = fopen(nodeseldata->trjfname, "a");

   /* create feat */
   nodeseldata->feat = NULL;
   SCIP_CALL( SCIPfeatCreate(scip, &nodeseldata->feat, SCIP_FEATNODESEL_SIZE) );
   assert(nodeseldata->feat != NULL);
   SCIPfeatSetMaxDepth(nodeseldata->feat, (SCIP_Real)SCIPgetNVars(scip));
  
   /* create optimal node feat */
   nodeseldata->optfeat = NULL;
   SCIP_CALL( SCIPfeatCreate(scip, &nodeseldata->optfeat, SCIP_FEATNODESEL_SIZE) );
   assert(nodeseldata->optfeat != NULL);
   SCIPfeatSetMaxDepth(nodeseldata->optfeat, SCIPgetNVars(scip));

   nodeseldata->optnodenumber = -1;
   nodeseldata->negate = TRUE;

   return SCIP_OKAY;
}

/** destructor of node selector to free user data (called when SCIP is exiting) */
static
SCIP_DECL_NODESELFREE(nodeselFreeOracle)
{
   SCIP_NODESELDATA* nodeseldata;
   assert(scip != NULL);
   assert(nodesel != NULL);

   nodeseldata = SCIPnodeselGetData(nodesel);

   assert(nodeseldata->optsol != NULL);
   SCIP_CALL( SCIPfreeSolSelf(scip, &nodeseldata->optsol) );
  
   if( nodeseldata->trjfile != NULL)
      fclose(nodeseldata->trjfile);

   if( nodeseldata->feat != NULL )
      SCIP_CALL( SCIPfeatFree(scip, &nodeseldata->feat) );
   if( nodeseldata->optfeat != NULL )
      SCIP_CALL( SCIPfeatFree(scip, &nodeseldata->optfeat) );
   
   SCIPfreeBlockMemory(scip, &nodeseldata);

   SCIPnodeselSetData(nodesel, NULL);

   return SCIP_OKAY;
}

/** node selection method of node selector */
static
SCIP_DECL_NODESELSELECT(nodeselSelectOracle)
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

   optchild = -1;
   for( i = 0; i < nchildren; i++)
   {
      SCIPnodeCheckOptimal(scip, children[i], nodeseldata->optsol); 
      SCIPnodeSetOptchecked(children[i]);
      if( SCIPnodeIsOptimal(children[i]) )
      {
         SCIPdebugMessage("opt node #%"SCIP_LONGINT_FORMAT"\n", SCIPnodeGetNumber(children[i]));
         nodeseldata->optnodenumber = SCIPnodeGetNumber(children[i]);
         optchild = i;
      }
   }

   /* write examples */
#ifndef SCIP_DEBUG
   if( nodeseldata->trjfile != NULL )
   {
#endif
      SCIPdebugMessage("node selection feature\n");
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
               SCIPfeatDiffLIBSVMPrint(scip, nodeseldata->trjfile, nodeseldata->optfeat, nodeseldata->feat, 1, nodeseldata->negate);
               SCIPfeatDiffLIBSVMPrint(scip, NULL, nodeseldata->optfeat, nodeseldata->feat, 1, nodeseldata->negate);
            }
         }
         for( i = 0; i < nsiblings; i++ )
         {
            SCIPcalcNodeselFeat(scip, siblings[i], nodeseldata->feat);
            nodeseldata->negate ^= 1;
            SCIPfeatDiffLIBSVMPrint(scip, nodeseldata->trjfile, nodeseldata->optfeat, nodeseldata->feat, 1, nodeseldata->negate);
            SCIPfeatDiffLIBSVMPrint(scip, NULL, nodeseldata->optfeat, nodeseldata->feat, 1, nodeseldata->negate);
         }
         for( i = 0; i < nleaves; i++ )
         {
            SCIPcalcNodeselFeat(scip, leaves[i], nodeseldata->feat);
            nodeseldata->negate ^= 1;
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
            SCIPfeatDiffLIBSVMPrint(scip, nodeseldata->trjfile, nodeseldata->optfeat, nodeseldata->feat, 1, nodeseldata->negate);
            SCIPfeatDiffLIBSVMPrint(scip, NULL, nodeseldata->optfeat, nodeseldata->feat, 1, nodeseldata->negate);
         }
      }
#ifndef SCIP_DEBUG
   }
#endif

   *selnode = SCIPgetBestNode(scip);

   return SCIP_OKAY;
}

/** node comparison method of oracle node selector */
static
SCIP_DECL_NODESELCOMP(nodeselCompOracle)
{  /*lint --e{715}*/
   SCIP_Bool isoptimal1;
   SCIP_Bool isoptimal2;

   assert(nodesel != NULL);
   assert(strcmp(SCIPnodeselGetName(nodesel), NODESEL_NAME) == 0);
   assert(scip != NULL);

   assert(SCIPnodeIsOptchecked(node1) == TRUE);
   assert(SCIPnodeIsOptchecked(node2) == TRUE);

   isoptimal1 = SCIPnodeIsOptimal(node1);
   isoptimal2 = SCIPnodeIsOptimal(node2);

   if( isoptimal1 == TRUE )
   {
      assert(isoptimal2 != TRUE);
      return -1;
   }
   else if( isoptimal2 == TRUE )
      return +1;
   else
   {
      int depth1;
      int depth2;

      depth1 = SCIPnodeGetDepth(node1);
      depth2 = SCIPnodeGetDepth(node2);
      if( depth1 > depth2 )
         return -1;
      else if( depth1 < depth2 )
         return +1;
      else
      {
         SCIP_Real lowerbound1;
         SCIP_Real lowerbound2;

         lowerbound1 = SCIPnodeGetLowerbound(node1);
         lowerbound2 = SCIPnodeGetLowerbound(node2);
         if( SCIPisLT(scip, lowerbound1, lowerbound2) )
            return -1;
         else if( SCIPisGT(scip, lowerbound1, lowerbound2) )
            return +1;
         if( lowerbound1 < lowerbound2 )
            return -1;
         else if( lowerbound1 > lowerbound2 )
            return +1;
         else
            return 0;
      }
   }
}

/*
 * node selector specific interface methods
 */

/** creates the uct node selector and includes it in SCIP */
SCIP_RETCODE SCIPincludeNodeselOracle(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_NODESELDATA* nodeseldata;
   SCIP_NODESEL* nodesel;

   /* create oracle node selector data */
   SCIP_CALL( SCIPallocBlockMemory(scip, &nodeseldata) );

   nodesel = NULL;
   nodeseldata->optsol = NULL;
   nodeseldata->solfname = NULL;

   /* use SCIPincludeNodeselBasic() plus setter functions if you want to set callbacks one-by-one and your code should
    * compile independent of new callbacks being added in future SCIP versions
    */
   SCIP_CALL( SCIPincludeNodeselBasic(scip, &nodesel, NODESEL_NAME, NODESEL_DESC, NODESEL_STDPRIORITY,
          NODESEL_MEMSAVEPRIORITY, nodeselSelectOracle, nodeselCompOracle, nodeseldata) );

   assert(nodesel != NULL);

   /* set non fundamental callbacks via setter functions */
   SCIP_CALL( SCIPsetNodeselCopy(scip, nodesel, nodeselCopyOracle) );
   SCIP_CALL( SCIPsetNodeselInitsol(scip, nodesel, nodeselInitsolOracle) );
   SCIP_CALL( SCIPsetNodeselFree(scip, nodesel, nodeselFreeOracle) );

   /* add oracle node selector parameters */
   SCIP_CALL( SCIPaddStringParam(scip, 
         "nodeselection/"NODESEL_NAME"/solfname",
         "name of the optimal solution file",
         &nodeseldata->solfname, FALSE, DEFAULT_FILENAME, NULL, NULL) );
   SCIP_CALL( SCIPaddStringParam(scip, 
         "nodeselection/"NODESEL_NAME"/trjfname",
         "name of the file to write node selection trajectories",
         &nodeseldata->trjfname, FALSE, DEFAULT_FILENAME, NULL, NULL) );

   return SCIP_OKAY;
}
