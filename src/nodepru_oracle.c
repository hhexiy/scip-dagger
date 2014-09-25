/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the program and library             */
/*         SCIP --- Solving Constraint Integer Programs                      */
/*                                                                           */
/*    Copyright (C) 2002-2014 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SCIP is distributed under the terms of the ZIB Academic License.         */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SCIP; see the file COPYING. If not email to scip@zib.de.      */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**@file   nodepru_oracle.c
 * @brief  oracle node pruner which prunes all non-optimal node 
 * @author He He 
 *
 * the UCT node selection rule selects the next leaf according to a mixed score of the node's actual lower bound
 *
 * @note It should be avoided to switch to uct node selection after the branch and bound process has begun because
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include <assert.h>
#include <string.h>
#include "nodepru_oracle.h"
#include "nodesel_oracle.h"
#include "scip/sol.h"
#include "scip/struct_set.h"

#define NODEPRU_NAME            "oracle"
#define NODEPRU_DESC            "node pruner which always prunes non-optimal nodes"
#define NODEPRU_STDPRIORITY     10
#define NODEPRU_MEMSAVEPRIORITY 0

#define DEFAULT_FILENAME        ""

/*
 * Data structures
 */

/** node pruner data */
struct SCIP_NodepruData
{
   SCIP_SOL*          optsol;             /**< optimal solution */
   char*              solfname;           /**< name of the solution file */
};

/*
 * Callback methods of node pruner
 */

/** copy method for node pruner plugins (called when SCIP copies plugins) */
static
SCIP_DECL_NODEPRUCOPY(nodepruCopyOracle)
{  /*lint --e{715}*/
   assert(scip != NULL);
   SCIP_CALL( SCIPincludeNodepruOracle(scip) );

   return SCIP_OKAY;
}

/** solving process initialization method of node pruner (called when branch and bound process is about to begin) */
static
SCIP_DECL_NODEPRUINITSOL(nodepruInitsolOracle)
{
   SCIP_NODEPRUDATA* nodeprudata;
   assert(scip != NULL);
   assert(nodepru != NULL);

   nodeprudata = SCIPnodepruGetData(nodepru);

   assert(nodeprudata != NULL);
   /** solfname should be set before including nodepruoracle */
   assert(nodeprudata->solfname != NULL);
   nodeprudata->optsol = NULL;

   SCIP_CALL( SCIPreadOptSol(scip, nodeprudata->solfname, &nodeprudata->optsol) );
   assert(nodeprudata->optsol != NULL);
  
   SCIP_CALL( SCIPprintSol(scip, nodeprudata->optsol, NULL, FALSE) ); 

   return SCIP_OKAY;
}

/** destructor of node pruner to free user data (called when SCIP is exiting) */
static
SCIP_DECL_NODEPRUFREE(nodepruFreeOracle)
{
   SCIP_NODEPRUDATA* nodeprudata;
   assert(scip != NULL);
   assert(nodepru != NULL);

   nodeprudata = SCIPnodepruGetData(nodepru);

   assert(nodeprudata->optsol != NULL);
   SCIP_CALL( SCIPfreeSolSelf(scip, &nodeprudata->optsol) );

   SCIPfreeBlockMemory(scip, &nodeprudata);

   SCIPnodepruSetData(nodepru, NULL);

   return SCIP_OKAY;
}

/** node pruning method of node pruner */
static
SCIP_DECL_NODEPRUPRUNE(nodepruPruneOracle)
{
   SCIP_NODEPRUDATA* nodeprudata;
   SCIP_SOL* optsol;

   assert(nodepru != NULL);
   assert(strcmp(SCIPnodepruGetName(nodepru), NODEPRU_NAME) == 0);
   assert(scip != NULL);
   assert(node != NULL);

   nodeprudata = SCIPnodepruGetData(nodepru);
   assert(nodeprudata != NULL);
   optsol = nodeprudata->optsol;
   assert(optsol != NULL);

   if( SCIPnodeCheckOptimal(scip, optsol, node) )
      *prune = FALSE;
   else
      *prune = TRUE;
   
   return SCIP_OKAY;
}

/*
 * node pruner specific interface methods
 */

/** creates the uct node pruner and includes it in SCIP */
SCIP_RETCODE SCIPincludeNodepruOracle(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_NODEPRUDATA* nodeprudata;
   SCIP_NODEPRU* nodepru;

   /* create oracle node pruner data */
   SCIP_CALL( SCIPallocBlockMemory(scip, &nodeprudata) );

   nodepru = NULL;
   nodeprudata->optsol = NULL;
   nodeprudata->solfname = NULL;

   /* use SCIPincludeNodepruBasic() plus setter functions if you want to set callbacks one-by-one and your code should
    * compile independent of new callbacks being added in future SCIP versions
    */
   SCIP_CALL( SCIPincludeNodepruBasic(scip, &nodepru, NODEPRU_NAME, NODEPRU_DESC, NODEPRU_STDPRIORITY,
          NODEPRU_MEMSAVEPRIORITY, nodepruPruneOracle, nodeprudata) );

   assert(nodepru != NULL);

   /* set non fundamental callbacks via setter functions */
   SCIP_CALL( SCIPsetNodepruCopy(scip, nodepru, nodepruCopyOracle) );
   SCIP_CALL( SCIPsetNodepruInitsol(scip, nodepru, nodepruInitsolOracle) );
   SCIP_CALL( SCIPsetNodepruFree(scip, nodepru, nodepruFreeOracle) );

   /* add oracle node pruner parameters */
   SCIP_CALL( SCIPaddStringParam(scip, 
         "nodepruning/"NODEPRU_NAME"/solfname",
         "name of the optimal solution file",
         &nodeprudata->solfname, FALSE, DEFAULT_FILENAME, NULL, NULL) );

   return SCIP_OKAY;
}
