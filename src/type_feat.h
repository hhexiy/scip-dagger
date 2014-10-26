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
/* TODO: remove inf; scale of objconstr is off; add relative bounds to parent node? */
enum SCIP_FeatNodesel
{
   SCIP_FEATNODESEL_LOWERBOUND               = 0,
   SCIP_FEATNODESEL_ESTIMATE                 = 1,
   SCIP_FEATNODESEL_TYPE_SIBLING             = 2,
   SCIP_FEATNODESEL_TYPE_CHILD               = 3,
   SCIP_FEATNODESEL_TYPE_LEAF                = 4,
   SCIP_FEATNODESEL_BRANCHVAR_BOUNDLPDIFF    = 5,
   SCIP_FEATNODESEL_BRANCHVAR_ROOTLPDIFF     = 6,
   SCIP_FEATNODESEL_BRANCHVAR_PRIO_UP        = 7,
   SCIP_FEATNODESEL_BRANCHVAR_PRIO_DOWN      = 8,
   SCIP_FEATNODESEL_BRANCHVAR_PSEUDOCOST     = 9,
   SCIP_FEATNODESEL_BRANCHVAR_INF            = 10,
   SCIP_FEATNODESEL_RELATIVEBOUND            = 11,
   SCIP_FEATNODESEL_GLOBALUPPERBOUND         = 12, 
   SCIP_FEATNODESEL_GAP                      = 13,
   SCIP_FEATNODESEL_GAPINF                   = 14,
   SCIP_FEATNODESEL_GLOBALUPPERBOUNDINF      = 15, 
   SCIP_FEATNODESEL_PLUNGEDEPTH              = 16,
   SCIP_FEATNODESEL_RELATIVEDEPTH            = 17
};
typedef enum SCIP_FeatNodesel SCIP_FEATNODESEL;     /**< feature of node */

/** node pruner features */
/** features are respective to the depth and the branch direction */
enum SCIP_FeatNodepru
{
   SCIP_FEATNODEPRU_GLOBALLOWERBOUND         = 0,
   SCIP_FEATNODEPRU_GLOBALUPPERBOUND         = 1, 
   SCIP_FEATNODEPRU_GAP                      = 2,
   SCIP_FEATNODEPRU_NSOLUTION                = 3,
   SCIP_FEATNODEPRU_PLUNGEDEPTH              = 4,
   SCIP_FEATNODEPRU_RELATIVEDEPTH            = 5,
   SCIP_FEATNODEPRU_RELATIVEBOUND            = 6,
   SCIP_FEATNODEPRU_RELATIVEESTIMATE         = 7,
   SCIP_FEATNODEPRU_GAPINF                   = 8,
   SCIP_FEATNODEPRU_GLOBALUPPERBOUNDINF      = 9, 
   SCIP_FEATNODEPRU_BRANCHVAR_BOUNDLPDIFF    = 10,
   SCIP_FEATNODEPRU_BRANCHVAR_ROOTLPDIFF     = 11,
   SCIP_FEATNODEPRU_BRANCHVAR_PRIO_UP        = 12,
   SCIP_FEATNODEPRU_BRANCHVAR_PRIO_DOWN      = 13,
   SCIP_FEATNODEPRU_BRANCHVAR_PSEUDOCOST     = 14,
   SCIP_FEATNODEPRU_BRANCHVAR_INF            = 15 
};
typedef enum SCIP_FeatNodepru SCIP_FEATNODEPRU;     /**< feature of node */


typedef struct SCIP_Feat SCIP_FEAT;

#define SCIP_FEATNODESEL_SIZE 18 
#define SCIP_FEATNODEPRU_SIZE 16 

#ifdef __cplusplus
}
#endif

#endif
