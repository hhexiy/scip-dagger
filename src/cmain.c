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

/**@file   cmain.c
 * @brief  Main file for binpacking pricing example
 * @author Timo Berthold
 * @author Stefan Heinz
 *
 *  This the file contains the \ref main() main function of the projects. This includes all the default plugins of
 *  \SCIP and the once which belong to that projects. After that is starts the interactive shell of \SCIP or processes
 *  the shell arguments if given.
 */
#include <stdio.h>
#include <string.h>

#include "scip/scip.h"
#include "scip/scipshell.h"
#include "scip/scipdefplugins.h"

#include "nodesel_oracle.h"
#include "nodepru_oracle.h"

static
SCIP_RETCODE fromCommandLine(
   SCIP*                 scip,               /**< SCIP data structure */
   const char*           filename            /**< input file name */
   )
{
   SCIP_RETCODE retcode;

   /********************
    * Problem Creation *
    ********************/

   /** @note The message handler should be only fed line by line such the message has the chance to add string in front
    *        of each message
    */
   SCIPinfoMessage(scip, NULL, "\n");
   SCIPinfoMessage(scip, NULL, "read problem <%s>\n", filename);
   SCIPinfoMessage(scip, NULL, "============\n");
   SCIPinfoMessage(scip, NULL, "\n");


   retcode = SCIPreadProb(scip, filename, NULL);

   switch( retcode )
   {
   case SCIP_NOFILE:
      SCIPinfoMessage(scip, NULL, "file <%s> not found\n", filename);
      return SCIP_OKAY;
   case SCIP_PLUGINNOTFOUND:
      SCIPinfoMessage(scip, NULL, "no reader for input file <%s> available\n", filename);
      return SCIP_OKAY;
   case SCIP_READERROR:
      SCIPinfoMessage(scip, NULL, "error reading file <%s>\n", filename);
      return SCIP_OKAY;
   default:
      SCIP_CALL( retcode );
   } /*lint !e788*/

   /*******************
    * Problem Solving *
    *******************/

   /* solve problem */
   SCIPinfoMessage(scip, NULL, "\nsolve problem\n");
   SCIPinfoMessage(scip, NULL, "=============\n\n");

   SCIP_CALL( SCIPsolve(scip) );

   SCIPinfoMessage(scip, NULL, "\nprimal solution:\n");
   SCIPinfoMessage(scip, NULL, "================\n\n");
   SCIP_CALL( SCIPprintBestSol(scip, NULL, FALSE) );


   /**************
    * Statistics *
    **************/

   /*
   SCIPinfoMessage(scip, NULL, "\nStatistics\n");
   SCIPinfoMessage(scip, NULL, "==========\n\n");

   SCIP_CALL( SCIPprintStatistics(scip, NULL) );
   */

   return SCIP_OKAY;
}

/** evaluates command line parameters */
static
SCIP_RETCODE processShellArguments(
   SCIP*                 scip,               /**< SCIP data structure */
   int                   argc,               /**< number of shell parameters */
   char**                argv,               /**< array with shell parameters */
   const char*           defaultsetname      /**< name of default settings file */
   )
{  /*lint --e{850}*/
   char* probname = NULL;
   char* solname = NULL;
   char* settingsname = NULL;
   char* logname = NULL;
   SCIP_Bool quiet;
   SCIP_Bool paramerror;
   int i;

   /********************
    * Parse parameters *
    ********************/

   quiet = FALSE;
   paramerror = FALSE;
   for( i = 1; i < argc; ++i )
   {
      if( strcmp(argv[i], "-l") == 0 )
      {
         i++;
         if( i < argc )
            logname = argv[i];
         else
         {
            printf("missing log filename after parameter '-l'\n");
            paramerror = TRUE;
         }
      }
      else if( strcmp(argv[i], "-q") == 0 )
         quiet = TRUE;
      else if( strcmp(argv[i], "-s") == 0 )
      {
         i++;
         if( i < argc )
            settingsname = argv[i];
         else
         {
            printf("missing settings filename after parameter '-s'\n");
            paramerror = TRUE;
         }
      }
      else if( strcmp(argv[i], "-f") == 0 )
      {
         i++;
         if( i < argc )
            probname = argv[i];
         else
         {
            printf("missing problem filename after parameter '-f'\n");
            paramerror = TRUE;
         }
      }
      else if( strcmp(argv[i], "-o") == 0 )
      {
         i++;
         if( i < argc )
            solname = argv[i];
         else
         {
            printf("missing optimal solution filename after parameter '-o'\n");
            paramerror = TRUE;
         }
      }
      else
      {
         printf("invalid parameter <%s>\n", argv[i]);
         paramerror = TRUE;
      }
   }

   if( !paramerror )
   {
      /***********************************
       * create log file message handler *
       ***********************************/

      if( quiet )
      {
         SCIPsetMessagehdlrQuiet(scip, quiet);
      }

      if( logname != NULL )
      {
         SCIPsetMessagehdlrLogfile(scip, logname);
      }

      if ( solname != NULL )
      {
         SCIP_CALL( SCIPsetStringParam(scip, "nodeselection/oracle/solfname", solname) );
         SCIP_CALL( SCIPsetStringParam(scip, "nodepruning/oracle/solfname", solname) );
      }
         
      SCIP_CALL( SCIPsetStringParam(scip, "vbc/filename", "test.vbc") );

      /***********************************
       * Version and library information *
       ***********************************/

      SCIPprintVersion(scip, NULL);
      SCIPinfoMessage(scip, NULL, "\n");

      SCIPprintExternalCodes(scip, NULL);
      SCIPinfoMessage(scip, NULL, "\n");

      /*****************
       * Load settings *
       *****************/

      if( settingsname != NULL )
      {
         SCIP_CALL( SCIPreadParams(scip, settingsname) );
      }
      else if( defaultsetname != NULL )
      {
         SCIP_CALL( SCIPreadParams(scip, defaultsetname) );
      }

      /**************
       * Start SCIP *
       **************/

      if( probname != NULL )
      {
         SCIP_CALL( fromCommandLine(scip, probname) );
      }
      else
      {
         return SCIP_OKAY;
      }

   }
   else
   {
      printf("\nsyntax: %s [-l <logfile>] [-q] [-s <settings>] [-f <problem>] [-o <solution>]\n"
         "  -l <logfile>  : copy output into log file\n"
         "  -q            : suppress screen messages\n"
         "  -s <settings> : load parameter settings (.set) file\n"
         "  -f <problem>  : load and solve problem file\n"
         "  -o <solution> : load optimal solution file\n",
         argv[0]);
   }

   return SCIP_OKAY;
}

/** creates a SCIP instance with default plugins, evaluates command line parameters, runs SCIP appropriately,
 *  and frees the SCIP instance
 */
static
SCIP_RETCODE runShell(
   int                        argc,               /**< number of shell parameters */
   char**                     argv,               /**< array with shell parameters */
   const char*                defaultsetname      /**< name of default settings file */
   )
{
   SCIP* scip = NULL;
   int num_heur = 0;
   SCIP_HEUR** heurs = NULL;
   SCIP_BRANCHRULE* branch_rule = NULL;
   SCIP_NODESEL* nodesel = NULL;
   int i;

   /*********
    * Setup *
    *********/

   /* initialize SCIP */
   SCIP_CALL( SCIPcreate(&scip) );
   
   /* include default SCIP plugins */
   SCIP_CALL( SCIPincludeDefaultPlugins(scip) );

   /* include oracle node selector */
   SCIP_CALL( SCIPincludeNodeselOracle(scip) );

   /* include oracle node pruner */
   SCIP_CALL( SCIPincludeNodepruOracle(scip) );

   /* disable heuristics */
   num_heur = SCIPgetNHeurs( scip );
   heurs = SCIPgetHeurs( scip );
   for( i = 0; i < num_heur; i++ )
   {
      SCIPheurSetFreq( heurs[i], -1 );
   }

   /* use most infeasible branching */
   branch_rule = SCIPfindBranchrule( scip, "mostinf" );
   SCIP_CALL( SCIPsetBranchrulePriority( scip, branch_rule, 9999999 ) );
   
   /* use oracle node selector */
   nodesel = SCIPfindNodesel( scip, "oracle" );
   SCIP_CALL( SCIPsetNodeselStdPriority( scip, nodesel, 9999999 ) );
   
   /**********************************
    * Process command line arguments *
    **********************************/
   SCIP_CALL( processShellArguments(scip, argc, argv, defaultsetname) );

   /********************
    * Deinitialization *
    ********************/

   SCIP_CALL( SCIPfree(&scip) );

   BMScheckEmptyMemory();

   return SCIP_OKAY;
}

int
main(
   int                        argc,
   char**                     argv
   )
{
   SCIP_RETCODE retcode;

   retcode = runShell(argc, argv, NULL);
   if( retcode != SCIP_OKAY )
   {
      SCIPprintError(retcode);
      return -1;
   }

   return 0;
}

