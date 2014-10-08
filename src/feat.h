/**@file   feat.h
 * @brief  internal methods for node features 
 * @author He He 
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_FEAT_H__
#define __SCIP_FEAT_H__

#include "scip/def.h"
#include "scip/scip.h"
#include "scip/type_lp.h"
#include "pub_feat.h"

#ifdef NDEBUG
#include "struct_feat.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** write feature vector in libsvm format */
extern
void SCIPfeatLIBSVMPrint(
   SCIP*             scip,
   FILE*             file,
   SCIP_FEAT*        feat,
   int               label
   );

/** write feature vector diff (feat1 - feat2) in libsvm format */
extern
void SCIPfeatDiffLIBSVMPrint(
   SCIP*             scip,
   FILE*             file,
   SCIP_FEAT*        feat1,
   SCIP_FEAT*        feat2,
   int               label,
   SCIP_Bool         negate
   );

/** calculate feature values for the node pruner of this node */
void SCIPcalcNodepruFeat(
   SCIP*             scip,
   SCIP_NODE*        node,
   SCIP_FEAT*        feat
   );

/** calculate feature values for the node selector of this node */
extern
void SCIPcalcNodeselFeat(
   SCIP*             scip,
   SCIP_NODE*        node,
   SCIP_FEAT*        feat
   );

/** returns offset of the feature index */
extern
int SCIPfeatGetOffset(
   SCIP_FEAT* feat
   );

/** create feature vector and normalizers, initialized to zero */
extern
SCIP_RETCODE SCIPfeatCreate(
   SCIP*                scip,
   SCIP_FEAT**          feat,
   int                  featsize
   );

/** copy feature vector value */
extern
void SCIPfeatCopy(
   SCIP_FEAT*           feat,
   SCIP_FEAT*           sourcefeat 
   );

/** free feature vector */
extern
SCIP_RETCODE SCIPfeatFree(
   SCIP*                scip,
   SCIP_FEAT**          feat 
   );

#ifdef NDEBUG

/* In optimized mode, the function calls are overwritten by defines to reduce the number of function calls and
 * speed up the algorithms.
 */

#define SCIPfeatGetOffset(feat)     (feat->size * 2) * (feat->depth / (feat->maxdepth / 10)) + (feat->size * (int)feat->boundtype)

#endif

#ifdef __cplusplus
}
#endif

#endif
