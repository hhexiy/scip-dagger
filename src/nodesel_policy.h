/**@file   nodesel_policy.h
 * @ingroup NODESELECTORS
 * @brief  
 * @author He He 
 *
 * the UCT node selection rule selects the next leaf according to a mixed score of the node's actual lower bound
 * and the number of times it has been visited so far compared to its parent node.
 *
 * The idea of UCT node selection for MIP appeared in:
 * Ashish Sabharwal and Horst Samulowitz
 * Guiding Combinatorial Optimization with UCT (2011)
 *
 * @note It should be avoided to switch to uct node selection after the branch and bound process has begun because
 *       the central UCT score information how often a path was taken is not collected if UCT is inactive. A safe use of
 *       UCT is to switch it on before SCIP starts optimization.
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_NODESEL_POLICY_H__
#define __SCIP_NODESEL_POLICY_H__


#include "scip/scip.h"
#include "feat.h"

#ifdef __cplusplus
extern "C" {
#endif

/** creates the policy node selector and includes it in SCIP */
EXTERN
SCIP_RETCODE SCIPincludeNodeselPolicy(
   SCIP*                 scip                /**< SCIP data structure */
   );

EXTERN
void SCIPnodeselpolicyPrintStatistics(
   SCIP*                 scip,
   SCIP_NODESEL*         nodesel,
   FILE*                 file
   );

#ifdef __cplusplus
}
#endif

#endif
