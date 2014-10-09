/**@file   nodepru_dagger.h
 * @ingroup NODEPRUNERS
 * @brief  
 * @author He He 
 *
 * the UCT node pruection rule pruects the next leaf according to a mixed score of the node's actual lower bound
 * and the number of times it has been visited so far compared to its parent node.
 *
 * The idea of UCT node pruection for MIP appeared in:
 * Ashish Sabharwal and Horst Samulowitz
 * Guiding Combinatorial Optimization with UCT (2011)
 *
 * @note It should be avoided to switch to uct node pruection after the branch and bound process has begun because
 *       the central UCT score information how often a path was taken is not collected if UCT is inactive. A safe use of
 *       UCT is to switch it on before SCIP starts optimization.
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_NODEPRU_DAGGER_H__
#define __SCIP_NODEPRU_DAGGER_H__


#include "scip/scip.h"
#include "feat.h"

#ifdef __cplusplus
extern "C" {
#endif

/** creates the dagger node pruner and includes it in SCIP */
EXTERN
SCIP_RETCODE SCIPincludeNodepruDagger(
   SCIP*                 scip                /**< SCIP data structure */
   );

EXTERN
void SCIPnodeprudaggerPrintStatistics(
   SCIP*                 scip,
   SCIP_NODEPRU*         nodepru,
   FILE*                 file
   );

#ifdef __cplusplus
}
#endif

#endif
