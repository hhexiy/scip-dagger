/**@file   policy.c
 * @brief  methods for policy
 * @author He He 
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include "scip/def.h"
#include "feat.h"
#include "policy.h"

#define HEADERSIZE_LIBSVM       6 

SCIP_RETCODE SCIPpolicyCreate(
   SCIP*              scip,
   SCIP_POLICY**      policy
   )
{
   assert(scip != NULL);
   assert(policy != NULL);

   SCIP_CALL( SCIPallocBlockMemory(scip, policy) );
   (*policy)->weights = NULL;
   (*policy)->size = 0;

   return SCIP_OKAY;
}

SCIP_RETCODE SCIPpolicyFree(
   SCIP*              scip,
   SCIP_POLICY**      policy
   )
{
   assert(scip != NULL);
   assert(policy != NULL);
   assert((*policy)->weights != NULL);

   BMSfreeMemoryArray(&(*policy)->weights);
   SCIPfreeBlockMemory(scip, policy);

   return SCIP_OKAY;
}

SCIP_RETCODE SCIPreadLIBSVMPolicy(
   SCIP*              scip,
   char*              fname,
   SCIP_POLICY**      policy       
   )
{
   int nlines = 0;
   int i;
   char buffer[SCIP_MAXSTRLEN];

   FILE* file = fopen(fname, "r");
   if( file == NULL )
   {
      SCIPerrorMessage("cannot open file <%s> for reading\n", fname);
      SCIPprintSysError(fname);
      return SCIP_NOFILE;
   }

   /* find out weight vector size */
   while( fgets(buffer, (int)sizeof(buffer), file) != NULL )
      nlines++;
   /* don't count libsvm model header */
   assert(nlines >= HEADERSIZE_LIBSVM);
   (*policy)->size = nlines - HEADERSIZE_LIBSVM;
   fclose(file);
   if( (*policy)->size == 0 )
   {
      SCIPerrorMessage("empty policy model\n");
      return SCIP_NOFILE;
   }

   SCIP_CALL( SCIPallocMemoryArray(scip, &(*policy)->weights, (*policy)->size) );

   /* have to reopen to read weights */
   file = fopen(fname, "r");
   /* skip header */
   for( i = 0; i < HEADERSIZE_LIBSVM; i++ )
      fgets(buffer, (int)sizeof(buffer), file);
   for( i = 0; i < (*policy)->size; i++ )
      fscanf(file, "%"SCIP_REAL_FORMAT, &((*policy)->weights[i]));

   fclose(file);

   SCIPverbMessage(scip, SCIP_VERBLEVEL_NORMAL, NULL, "policy of size %d from file <%s> was %s\n",
      (*policy)->size, fname, "read, will be used in the dagger node selector");

   return SCIP_OKAY;
}

/** calculate score of a node given its feature and the policy weight vector */
void SCIPcalcNodeScore(
   SCIP_NODE*         node,
   SCIP_FEAT*         feat,
   SCIP_POLICY*       policy
   )
{
   int offset = SCIPfeatGetOffset(feat);
   int i;
   SCIP_Real score = 0;
   SCIP_Real* weights = policy->weights;
   SCIP_Real* featvals = SCIPfeatGetVals(feat);

   assert(offset + SCIPfeatGetSize(feat) <= policy->size);

   for( i = 0; i < SCIPfeatGetSize(feat); i++ )
      score += featvals[i] * weights[i+offset];

   SCIPnodeSetScore(node, score);
   SCIPdebugMessage("score of node  #%"SCIP_LONGINT_FORMAT": %f\n", SCIPnodeGetNumber(node), SCIPnodeGetScore(node));
}

