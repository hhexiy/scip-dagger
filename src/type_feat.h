/**@file   type_feat.h
 * @ingroup TYPEDEFINITIONS
 * @brief  type definitions for node features 
 * @author He He 
 *
 *  This file defines the interface for node feature implemented in C.
 *
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_TYPE_FEAT_H__
#define __SCIP_TYPE_FEAT_H__

#ifdef __cplusplus
extern "C" {
#endif

/** feature type */
enum SCIP_FeatType
{
   SCIP_FEATTYPE_NODESEL = 0,
   SCIP_FEATTYPE_NODEPRU = 1
};
typedef enum SCIP_FeatType SCIP_FEATTYPE; 

/** node selector features */
/** features are respective to the depth and the branch direction */
/* TODO: remove inf; scale of objconstr is off */
enum SCIP_FeatNodesel
{
   SCIP_FEATNODESEL_LOWERBOUND               = 0,
   SCIP_FEATNODESEL_ESTIMATE                 = 1,
   SCIP_FEATNODESEL_TYPE_SIBLING             = 2,
   SCIP_FEATNODESEL_TYPE_CHILD               = 3,
   SCIP_FEATNODESEL_TYPE_LEAF                = 4,
   SCIP_FEATNODESEL_BRANCHVAR_OBJCONSTR      = 5, 
   SCIP_FEATNODESEL_BRANCHVAR_BOUNDLPDIFF    = 6,
   SCIP_FEATNODESEL_BRANCHVAR_ROOTLPDIFF     = 7,
   SCIP_FEATNODESEL_BRANCHVAR_PRIO_UP        = 8,
   SCIP_FEATNODESEL_BRANCHVAR_PRIO_DOWN      = 9,
   SCIP_FEATNODESEL_BRANCHVAR_PSEUDOCOST     = 10,
   SCIP_FEATNODESEL_BRANCHVAR_INF            = 11,
   SCIP_FEATNODESEL_RELATIVEBOUND            = 12 
};
typedef enum SCIP_FeatNodesel SCIP_FEATNODESEL;     /**< feature of node */

typedef struct SCIP_Feat SCIP_FEAT;

#define SCIP_FEATNODESEL_SIZE 13 
#define SCIP_FEATNODEPRU_SIZE 12 

#ifdef __cplusplus
}
#endif

#endif
