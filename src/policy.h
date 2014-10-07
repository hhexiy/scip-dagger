/**@file   policy.h
 * @brief  internal methods for node policyures 
 * @author He He 
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_POLICY_H__
#define __SCIP_POLICY_H__

#include "scip/def.h"
#include "scip/scip.h"
#include "struct_policy.h"

#ifdef __cplusplus
extern "C" {
#endif

extern 
SCIP_RETCODE SCIPpolicyCreate(
   SCIP*              scip,
   SCIP_POLICY**      policy
   );

extern 
SCIP_RETCODE SCIPpolicyFree(
   SCIP*              scip,
   SCIP_POLICY**      policy
   );

/** read policy (model) in LIBSVM format */
extern
SCIP_RETCODE SCIPreadLIBSVMPolicy(
   SCIP*              scip,
   char*              fname,
   SCIP_POLICY**      policy
   );

/** calculate score of a node given its feature and the policy weight vector */
extern
void SCIPcalcNodeScore(
   SCIP_NODE*         node,
   SCIP_FEAT*         feat,
   SCIP_POLICY*       policy
   );

#ifdef __cplusplus
}
#endif

#endif
