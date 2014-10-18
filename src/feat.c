/**@file   feat.c
 * @brief  methods for node features 
 * @author He He 
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include "scip/def.h"
#include "feat.h"
#include "struct_feat.h"
#include "scip/tree.h"
#include "scip/var.h"
#include "scip/stat.h"
#include "scip/set.h"
#include "scip/struct_scip.h"
#include "math.h"

/** copy feature vector value */
void SCIPfeatCopy(
   SCIP_FEAT*           feat,
   SCIP_FEAT*           sourcefeat 
   )
{
   int i;
   
   assert(feat != NULL);
   assert(feat->vals != NULL);
   assert(sourcefeat != NULL);
   assert(sourcefeat->vals != NULL);

   sourcefeat->maxdepth = feat->maxdepth;
   sourcefeat->depth = feat->depth;
   sourcefeat->size = feat->size;
   sourcefeat->boundtype = feat->boundtype;
   sourcefeat->rootlpobj = feat->rootlpobj;
   sourcefeat->sumobjcoeff = feat->sumobjcoeff;
   sourcefeat->nconstrs = feat->nconstrs;

   for( i = 0; i < feat->size; i++ )
      sourcefeat->vals[i] = feat->vals[i];
}

/** create feature vector and normalizers, initialized to zero */
SCIP_RETCODE SCIPfeatCreate(
   SCIP*                scip,
   SCIP_FEAT**          feat,
   int                  size
   )
{
   int i;

   assert(scip != NULL);
   assert(feat != NULL);
   printf("create feat\n");
   SCIP_CALL( SCIPallocBlockMemory(scip, feat) );

   SCIP_ALLOC( BMSallocMemoryArray(&(*feat)->vals, size) );

   for( i = 0; i < size; i++ )
      (*feat)->vals[i] = 0;

   (*feat)->rootlpobj = 0;
   (*feat)->sumobjcoeff = 0;
   (*feat)->nconstrs = 0;
   (*feat)->maxdepth = 0;
   (*feat)->depth = 0;
   (*feat)->size = size;
   (*feat)->boundtype = 0;

   return SCIP_OKAY;
}

/** free feature vector */
SCIP_RETCODE SCIPfeatFree(
   SCIP*                scip,
   SCIP_FEAT**          feat 
   )
{
   assert(scip != NULL);
   assert(feat != NULL);
   assert(*feat != NULL);
   printf("free feat\n");
   BMSfreeMemoryArray(&(*feat)->vals);
   SCIPfreeBlockMemory(scip, feat);

   return SCIP_OKAY;
}

/** calculate feature values for the node pruner of this node */
void SCIPcalcNodepruFeat(
   SCIP*             scip,
   SCIP_NODE*        node,
   SCIP_FEAT*        feat
   )
{
   SCIP_Real lowerbound;
   SCIP_Real upperbound;
   SCIP_Bool upperboundinf;
   SCIP_Real rootlowerbound;
   SCIP_VAR* branchvar;
   SCIP_COL* branchvarcol;
   SCIP_BOUNDCHG* boundchgs;
   SCIP_BRANCHDIR branchdirpreferred;
   SCIP_Real branchbound;
   SCIP_Bool haslp;
   SCIP_Real varsol;
   SCIP_Real varrootsol;
   SCIP_Real varobj;                /**< coefficent in the objective function */

   assert(node != NULL);
   assert(SCIPnodeGetDepth(node) != 0);
   assert(feat != NULL);
   assert(feat->maxdepth != 0);

   boundchgs = node->domchg->domchgbound.boundchgs;
   assert(boundchgs != NULL);
   assert(boundchgs[0].boundchgtype == SCIP_BOUNDCHGTYPE_BRANCHING);

   feat->depth = SCIPnodeGetDepth(node);

   lowerbound = SCIPgetLowerbound(scip);
   assert(!SCIPsetIsInfinity(scip->set, lowerbound));

   upperbound = SCIPgetUpperbound(scip);
   if( SCIPsetIsInfinity(scip->set, upperbound)
      || SCIPsetIsInfinity(scip->set, -upperbound) )
      upperboundinf = TRUE;
   else
      upperboundinf = FALSE;

   rootlowerbound = REALABS(scip->stat->rootlowerbound);
   if( SCIPsetIsZero(scip->set, rootlowerbound) )
      rootlowerbound = 0.0001;
   assert(!SCIPsetIsInfinity(scip->set, rootlowerbound));

   /* currently only support branching on one variable */
   branchvar = boundchgs[0].var; 
   branchbound = boundchgs[0].newbound;
   branchdirpreferred = SCIPvarGetBranchDirection(branchvar);
   branchvarcol = SCIPvarGetCol(branchvar);
   varobj = SCIPcolGetObj(branchvarcol);

   haslp = SCIPtreeHasFocusNodeLP(scip->tree);
   varsol = SCIPvarGetSol(branchvar, haslp);
   varrootsol = SCIPvarGetRootSol(branchvar);

   feat->boundtype = boundchgs[0].boundtype;

   /* calculate features */
   /* global features */
   if( SCIPsetIsEQ(scip->set, upperbound, lowerbound) )
      feat->vals[SCIP_FEATNODEPRU_GAP] = 0;
   else if( SCIPsetIsZero(scip->set, lowerbound)
      || upperboundinf ) 
      feat->vals[SCIP_FEATNODEPRU_GAPINF] = 1;
   else
      feat->vals[SCIP_FEATNODEPRU_GAP] = (upperbound - lowerbound)/REALABS(lowerbound);

   feat->vals[SCIP_FEATNODEPRU_GLOBALLOWERBOUND] = lowerbound / rootlowerbound;
   if( upperboundinf )
      feat->vals[SCIP_FEATNODEPRU_GLOBALUPPERBOUNDINF] = 1;
   else
      feat->vals[SCIP_FEATNODEPRU_GLOBALUPPERBOUND] = upperbound / rootlowerbound;

   feat->vals[SCIP_FEATNODEPRU_NSOLUTION] = SCIPgetNSolsFound(scip);
   feat->vals[SCIP_FEATNODEPRU_PLUNGEDEPTH] = SCIPgetPlungeDepth(scip) / (SCIP_Real)feat->maxdepth;
   feat->vals[SCIP_FEATNODEPRU_RELATIVEDEPTH] = feat->depth / (SCIP_Real)feat->maxdepth;

   /* node features */
   if( upperboundinf )
      upperbound = lowerbound + 0.2 * (upperbound - lowerbound);
   feat->vals[SCIP_FEATNODEPRU_RELATIVEBOUND] = SCIPnodeGetLowerbound(node) / (upperbound - lowerbound);
   feat->vals[SCIP_FEATNODEPRU_RELATIVEESTIMATE] = SCIPnodeGetEstimate(node) / (upperbound - lowerbound);

   /* branch var features */
   feat->vals[SCIP_FEATNODESEL_BRANCHVAR_BOUNDLPDIFF] = branchbound - varsol;
   feat->vals[SCIP_FEATNODESEL_BRANCHVAR_ROOTLPDIFF] = varrootsol - varsol;

   if( branchdirpreferred == SCIP_BRANCHDIR_DOWNWARDS )
      feat->vals[SCIP_FEATNODESEL_BRANCHVAR_PRIO_DOWN] = 1;
   else if(branchdirpreferred == SCIP_BRANCHDIR_UPWARDS ) 
      feat->vals[SCIP_FEATNODESEL_BRANCHVAR_PRIO_UP] = 1;

   feat->vals[SCIP_FEATNODESEL_BRANCHVAR_PSEUDOCOST] = SCIPvarGetPseudocost(branchvar, scip->stat, branchbound - varsol) / ABS(varobj);

   feat->vals[SCIP_FEATNODESEL_BRANCHVAR_INF] = 
      feat->boundtype == SCIP_BOUNDTYPE_LOWER ? 
      SCIPvarGetAvgInferences(branchvar, scip->stat, SCIP_BRANCHDIR_UPWARDS) / (SCIP_Real)feat->maxdepth : 
      SCIPvarGetAvgInferences(branchvar, scip->stat, SCIP_BRANCHDIR_DOWNWARDS) / (SCIP_Real)feat->maxdepth;
}

/** calculate feature values for the node selector of this node */
void SCIPcalcNodeselFeat(
   SCIP*             scip,
   SCIP_NODE*        node,
   SCIP_FEAT*        feat
   )
{
   SCIP_NODETYPE nodetype;
   SCIP_Real nodelowerbound;
   SCIP_Real rootlowerbound;
   SCIP_Real lowerbound;            /**< global lower bound */
   SCIP_Real upperbound;           /**< global upper bound */
   SCIP_VAR* branchvar;
   SCIP_COL* branchvarcol;
   SCIP_BOUNDCHG* boundchgs;
   SCIP_BRANCHDIR branchdirpreferred;
   SCIP_Real branchbound;
   SCIP_Bool haslp;
   SCIP_Real varsol;
   SCIP_Real varrootsol;
   SCIP_Real varobj;                /**< coefficent in the objective function */
   int varcolsize;                  /**< number of nonzero entries in the column */

   assert(node != NULL);
   assert(SCIPnodeGetDepth(node) != 0);
   assert(feat != NULL);
   assert(feat->maxdepth != 0);

   boundchgs = node->domchg->domchgbound.boundchgs;
   assert(boundchgs != NULL);
   assert(boundchgs[0].boundchgtype == SCIP_BOUNDCHGTYPE_BRANCHING);

   /* extract necessary information */
   nodetype = SCIPnodeGetType(node);
   nodelowerbound = SCIPnodeGetLowerbound(node);
   rootlowerbound = REALABS(scip->stat->rootlowerbound);
   if( SCIPsetIsZero(scip->set, rootlowerbound) )
      rootlowerbound = 0.0001;
   assert(!SCIPsetIsInfinity(scip->set, rootlowerbound));
   lowerbound = SCIPgetLowerbound(scip);
   upperbound = SCIPgetUpperbound(scip);
   /* if we didn't find a solution yet, the upper bound is usually very bad:
    * use only 20% of the gap as upper bound
    */
   if( SCIPgetNSolsFound(scip) == 0 )
      upperbound = lowerbound + 0.2 * (upperbound - lowerbound);

   /* currently only support branching on one variable */
   branchvar = boundchgs[0].var; 
   branchbound = boundchgs[0].newbound;
   branchdirpreferred = SCIPvarGetBranchDirection(branchvar);
   branchvarcol = SCIPvarGetCol(branchvar);
   varobj = SCIPcolGetObj(branchvarcol);
   varcolsize = SCIPcolGetNNonz(branchvarcol);

   haslp = SCIPtreeHasFocusNodeLP(scip->tree);
   varsol = SCIPvarGetSol(branchvar, haslp);
   varrootsol = SCIPvarGetRootSol(branchvar);

   feat->depth = SCIPnodeGetDepth(node);
   feat->boundtype = boundchgs[0].boundtype;

   /* calculate features */
   feat->vals[SCIP_FEATNODESEL_LOWERBOUND] = 
      nodelowerbound / rootlowerbound;

   feat->vals[SCIP_FEATNODESEL_ESTIMATE] = 
      SCIPnodeGetEstimate(node) / rootlowerbound;

   if( !SCIPsetIsEQ(scip->set, upperbound, lowerbound) )
      feat->vals[SCIP_FEATNODESEL_RELATIVEBOUND] = (nodelowerbound - lowerbound) / (upperbound - lowerbound);

   if( nodetype == SCIP_NODETYPE_SIBLING )
      feat->vals[SCIP_FEATNODESEL_TYPE_SIBLING] = 1;
   else if( nodetype == SCIP_NODETYPE_CHILD )
      feat->vals[SCIP_FEATNODESEL_TYPE_CHILD] = 1;
   else if( nodetype == SCIP_NODETYPE_LEAF )
      feat->vals[SCIP_FEATNODESEL_TYPE_LEAF] = 1;

   if( varcolsize == 0 )
      feat->vals[SCIP_FEATNODESEL_BRANCHVAR_NOCONSTR] = 1;
   else
      feat->vals[SCIP_FEATNODESEL_BRANCHVAR_OBJCONSTR] = varobj / varcolsize;
   feat->vals[SCIP_FEATNODESEL_BRANCHVAR_BOUNDLPDIFF] = branchbound - varsol;
   feat->vals[SCIP_FEATNODESEL_BRANCHVAR_ROOTLPDIFF] = varrootsol - varsol;

   if( branchdirpreferred == SCIP_BRANCHDIR_DOWNWARDS )
      feat->vals[SCIP_FEATNODESEL_BRANCHVAR_PRIO_DOWN] = 1;
   else if(branchdirpreferred == SCIP_BRANCHDIR_UPWARDS ) 
      feat->vals[SCIP_FEATNODESEL_BRANCHVAR_PRIO_UP] = 1;

   feat->vals[SCIP_FEATNODESEL_BRANCHVAR_PSEUDOCOST] = SCIPvarGetPseudocost(branchvar, scip->stat, branchbound - varsol) / ABS(varobj);

   feat->vals[SCIP_FEATNODESEL_BRANCHVAR_INF] = 
      feat->boundtype == SCIP_BOUNDTYPE_LOWER ? 
      SCIPvarGetAvgInferences(branchvar, scip->stat, SCIP_BRANCHDIR_UPWARDS) / (SCIP_Real)feat->maxdepth : 
      SCIPvarGetAvgInferences(branchvar, scip->stat, SCIP_BRANCHDIR_DOWNWARDS) / (SCIP_Real)feat->maxdepth;

}

/** write feature vector diff (feat1 - feat2) in libsvm format */
void SCIPfeatDiffLIBSVMPrint(
   SCIP*             scip,
   FILE*             file,
   FILE*             wfile,
   SCIP_FEAT*        feat1,
   SCIP_FEAT*        feat2,
   int               label,
   SCIP_Bool         negate
   )
{
   int size;
   int i;
   int offset1;
   int offset2;
   SCIP_Real weight;

   assert(scip != NULL);
   assert(feat1 != NULL);
   assert(feat2 != NULL);
   assert(feat1->depth != 0);
   assert(feat2->depth != 0);
   assert(feat1->size == feat2->size);

   weight = SCIPfeatGetWeight(feat1);
   SCIPinfoMessage(scip, wfile, "%f\n", weight);  

   if( negate )
   {
      SCIP_FEAT* tmp = feat1;
      feat1 = feat2;
      feat2 = tmp;
      label = -1 * label;
   }

   size = SCIPfeatGetSize(feat1);
   offset1 = SCIPfeatGetOffset(feat1);
   offset2 = SCIPfeatGetOffset(feat2);

   SCIPinfoMessage(scip, file, "%d ", label);  

   if( offset1 == offset2 )
   {
      for( i = 0; i < size; i++ )
         SCIPinfoMessage(scip, file, "%d:%f ", i + offset1 + 1, feat1->vals[i] - feat2->vals[i]);
   }
   else
   {
      /* libsvm requires sorted indices, write smaller indices first */
      if( offset1 < offset2 )
      {
         /* feat1 */
         for( i = 0; i < size; i++ )
            SCIPinfoMessage(scip, file, "%d:%f ", i + offset1 + 1, feat1->vals[i]);
         /* -feat2 */
         for( i = 0; i < size; i++ )
            SCIPinfoMessage(scip, file, "%d:%f ", i + offset2 + 1, -feat2->vals[i]);
      }
      else
      {
         /* -feat2 */
         for( i = 0; i < size; i++ )
            SCIPinfoMessage(scip, file, "%d:%f ", i + offset2 + 1, -feat2->vals[i]);
         /* feat1 */
         for( i = 0; i < size; i++ )
            SCIPinfoMessage(scip, file, "%d:%f ", i + offset1 + 1, feat1->vals[i]);
      }
   }

   SCIPinfoMessage(scip, file, "\n");
}

/** write feature vector in libsvm format */
void SCIPfeatLIBSVMPrint(
   SCIP*             scip,
   FILE*             file,
   FILE*             wfile,
   SCIP_FEAT*        feat,
   int               label
   )
{
   int size;
   int i;
   int offset;
   SCIP_Real weight;

   assert(scip != NULL);
   assert(feat != NULL);
   assert(feat->depth != 0);

   weight = SCIPfeatGetWeight(feat);
   SCIPinfoMessage(scip, wfile, "%f\n", weight);  

   size = SCIPfeatGetSize(feat);
   offset = SCIPfeatGetOffset(feat);

   SCIPinfoMessage(scip, file, "%d ", label);  

   for( i = 0; i < size; i++ )
      SCIPinfoMessage(scip, file, "%d:%f ", i + offset + 1, feat->vals[i]);

   SCIPinfoMessage(scip, file, "\n");
}


/*
 * simple functions implemented as defines
 */

/* In debug mode, the following methods are implemented as function calls to ensure
 * type validity.
 * In optimized mode, the methods are implemented as defines to improve performance.
 * However, we want to have them in the library anyways, so we have to undef the defines.
 */

#undef SCIPfeatGetSize
#undef SCIPfeatGetVals
#undef SCIPfeatGetOffset
#undef SCIPfeatSetRootlpObj
#undef SCIPfeatSetSumObjCoeff
#undef SCIPfeatSetMaxDepth
#undef SCIPfeatSetNConstrs

void SCIPfeatSetRootlpObj(
   SCIP_FEAT*    feat,
   SCIP_Real     rootlpobj
   )
{
   assert(feat != NULL);

   feat->rootlpobj = rootlpobj;
}

SCIP_Real* SCIPfeatGetVals(
   SCIP_FEAT*    feat 
   )
{
   assert(feat != NULL);

   return feat->vals;
}

int SCIPfeatGetSize(
   SCIP_FEAT*    feat 
   )
{
   assert(feat != NULL);

   return feat->size;
}

void SCIPfeatSetSumObjCoeff(
   SCIP_FEAT*    feat,
   SCIP_Real     sumobjcoeff
   )
{
   assert(feat != NULL);

   feat->sumobjcoeff = sumobjcoeff;
}

void SCIPfeatSetMaxDepth(
   SCIP_FEAT*    feat,
   int           maxdepth
   )
{
   assert(feat != NULL);

   feat->maxdepth = maxdepth;
}

void SCIPfeatSetNConstrs(
   SCIP_FEAT*    feat,
   int           nconstrs 
   )
{
   assert(feat != NULL);

   feat->nconstrs = nconstrs;
}

/** returns the weight of the example */
SCIP_Real SCIPfeatGetWeight(
   SCIP_FEAT* feat
   )
{
   assert(feat != NULL);
   return 5 * exp(-(feat->depth-1) / (0.6*feat->maxdepth) * 1.61);
}

int SCIPfeatGetOffset(
   SCIP_FEAT* feat
   )
{
   assert(feat != NULL);
   return (feat->size * 2) * (feat->depth / (feat->maxdepth / 10)) + (feat->size * (int)feat->boundtype);
}

