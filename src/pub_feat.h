/**@file   pub_feat.h
 * @ingroup PUBLICMETHODS 
 * @brief  public methods for node features 
 * @author He He 
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_PUB_FEAT_H__
#define __SCIP_PUB_FEAT_H__

#include "scip/def.h"
#include "type_feat.h"

#ifdef NDEBUG
#include "struct_feat.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

EXTERN
int SCIPfeatGetSize(
   SCIP_FEAT*    feat 
   );

EXTERN
SCIP_Real* SCIPfeatGetVals(
   SCIP_FEAT*    feat 
   );

EXTERN
void SCIPfeatSetRootlpObj(
   SCIP_FEAT*    feat,
   SCIP_Real     rootlpobj
   );

EXTERN
void SCIPfeatSetMaxDepth(
   SCIP_FEAT*    feat,
   int           maxdepth
   );

EXTERN
void SCIPfeatSetSumObjCoeff(
   SCIP_FEAT*    feat,
   SCIP_Real     sumobjcoeff
   );

EXTERN
void SCIPfeatSetNConstrs(
   SCIP_FEAT*    feat,
   int           nconstrs 
   );


#ifdef NDEBUG

/* In optimized mode, the function calls are overwritten by defines to reduce the number of function calls and
 * speed up the algorithms.
 */

#define SCIPfeatGetSize(feat)     ((feat)->size)
#define SCIPfeatGetVals(feat)     ((feat)->vals)
#define SCIPfeatSetRootlpObj(feat, rootlpobj)     ((feat)->rootlpobj = (rootlpobj))
#define SCIPfeatSetSumObjCoeff(feat, sumobjcoeff)     ((feat)->sumobjcoeff = (sumobjcoeff))
#define SCIPfeatSetMaxDepth(feat, depth)     ((feat)->maxdepth = (depth))
#define SCIPfeatSetNConstrs(feat, nconstrs)     ((feat)->nconstrs = (nconstrs))

#endif

#ifdef __cplusplus
}
#endif

#endif
