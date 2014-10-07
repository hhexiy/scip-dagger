/**@file   struct_policy.h
 * @brief  data structures for node policyures 
 * @author He He 
 *
 *  This file defines the interface for node selector and pruner  policy implemented in C.
 *
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_STRUCT_POLICY_H__
#define __SCIP_STRUCT_POLICY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "scip/def.h"

/** policy for node selector and pruner */
struct SCIP_Policy
{
   SCIP_Real*     weights;
   int            size;
};
typedef struct SCIP_Policy SCIP_POLICY;

#ifdef __cplusplus
}
#endif

#endif
