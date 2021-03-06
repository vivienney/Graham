#include <sys/types.h>
#include <unistd.h>
#include <boost/filesystem.hpp>   
#include <boost/filesystem/fstream.hpp>
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string.hpp"
#include <iostream>
#include <cctype>
#include "Identifier.h"
#include "Config.h"
#include "Logger.h"
#include "dmmm_dbface.h"

#include "T_Stock.hpp"

#include "Financials.h"
#include "Test.h"

using namespace DMMM;
using namespace boost::filesystem;
using namespace std;
namespace nsBFS = boost::filesystem;

void 
handleBreak(int sig)
{
    exit(-1);
}

void
showHelpMessage(const string& exPath)
{
    cout << "usage: " << exPath 
         << " <command> <parameter>" << endl << endl
         << "Possible commands and how they're used:" << endl
         << endl;
}


void
dirFor(const path& logDir)
{
    if (!exists(logDir)){
      cout << "Creating log directory " << logDir << endl;
      create_directory(logDir);
    }
}

O_Stock
findStockByTicker( char* cticker )
{
    string ticker(cticker);
    T_Stock ts;
    return ts.select( ts._ticker() == ticker ).front();
}


int
mainMain(int argc, char* argv[])
{
    cout << "----------------" << endl;
    cout << "Running Edgar Data Retrival" << endl;

    path binPath(argv[0]);

    cout << "BinPath is " << binPath << endl;
    string command = argc >= 2 ? string(argv[1]) : string();

    if (command == string("--help") || command == string("-h")){
        showHelpMessage(argv[0]);
        exit(-1);
    }
    
    path basePath = binPath.remove_leaf() / "../";

    if (command == string("test")){
        cout << "\n ---  Running TEST mode ---"<< endl;
        path confFile = basePath / "test_conf.conf";
        cout << "Loading configuration file " << confFile.string() << endl;
        Config* config = new Config(confFile.string().c_str(), argc, argv);

        // set up test logger
        path logFile;
        path logDir;
        logDir = basePath / path(confParam<string>("log_file_name"));
        dirFor(logDir);
        logFile = logDir / string("test_log.txt");       
        cout << "Using test logger" << endl;
        Logger* logger = new Logger(logFile.string().c_str());

        // use TEST DB!!!
        string host = confParam<string>("db.host");
        string user = confParam<string>("db.user");
        string database = confParam<string>("db.database");
        string password = confParam<string>("db.password");
        DMMM::DBFace dbFace(database, host, user, password, logger->logFile());
        Test test;

        if ((argc >2) && (string(argv[2])=="-tmq"))
        {
            T_Stock ts;
            O_Stock stock = ts.select(ts._ticker() == string(argv[3]))[0];
            test.runGetQartersTM(stock);
            goto exitest;
        }

        if ((argc >2) && (string(argv[2])=="-reps"))
        {
            path pp = basePath / string("get_reports_issues.txt");       
            boost::filesystem::ofstream outFile(pp);

            T_Stock ts;
            vector<O_Stock> stocks;
            if (argc > 3)
                stocks = ts.select(ts._ticker() == string(argv[3]));
            else
                stocks = ts.select(ts._listed() == true && ts._id() >
                  ts.select(ts._ticker() == string("IMN")).front()._id());
            
            for( auto it = stocks.begin(); it != stocks.end();++it)
                test.getReportsTest( *it, outFile );

            outFile.close();
            goto exitest;
        } // end of handling -reps flag for test

        if (argc > 2)
        {       
            cout << "\n Callling company test method" << endl;
            string ticker(argv[2]);
            test.runCompanyTest( ticker );
        }
        else
            test.run_all();

      exitest:
        delete(config);
        delete(logger);
        exit(0);
    } // end of TEST ------------------------------------------


    path confFile = basePath / "conf.conf";
        
    cout << "Loading configuration file " << confFile.string() << endl;
    Config* config = new Config(confFile.string().c_str(), argc, argv);
        
    path logFile;
    path logDir;

    logDir = basePath /
        path(confParam<string>("log_file_name"));

    dirFor(logDir);
    logFile = logDir / string("log.txt");       
    Logger* logger = new Logger(logFile.string().c_str());
         
    LOG_INFO << "Analysing data " << confParam<string>("version");
    LOG_INFO << "Command line arguments: " << argv[0]; 

    string host = confParam<string>("db.host");
    string user = confParam<string>("db.user");
    string database = confParam<string>("db.database");
    string password = confParam<string>("db.password");
    DMMM::DBFace dbFace(database, host, user, password, logger->logFile());
        
// find rigt command to execute and call relavent comand methods
    if (command == string("update_financials")){

        if (std::isdigit( string(argv[2]).at(0) ) ){
            T_Stock ts;
            vector<O_Stock> stocks;
            string markStr = "updated_" + string(argv[2]);

            stocks = ts.select(ts._listed() == true && ts._mark() != markStr);

            for( auto it = stocks.begin(); it != stocks.end();++it)
            {
                EdgarData eData = EdgarData();
                cout << "\n\nUpdating financials for "<< it->_ticker() <<endl;

                try {
                    eData.updateFinancials(*it );
                    it->_mark() = markStr;
                    it->update();}
                catch(exception& e){
                    LOG_ERROR << "EXCEPTION CAUGHT - Updating " << it->_ticker() << "Caught exception " << e.what() ;
                }

            }
        } else {
            EdgarData eData = EdgarData();
            O_Stock stock = findStockByTicker( argv[2] );
            eData.updateFinancials( stock );
        }
    }
    if (command == string("get_single_year")){
        EdgarData eData = EdgarData();
        O_Stock stock = findStockByTicker( argv[2] );
        size_t year = stoi( argv[3]);
        if (!eData.getSingleYear( stock, year ))
            eData.getSingleYear( stock, year); // try again, if year was updated
    }
    if (command == string("get_all_income_statement_years")){
        EdgarData eData = EdgarData();

        O_Stock stock = findStockByTicker( argv[2] );
        string acn = argv[3];
        eData.getAllYearsFromIncome(stock, acn );
    }

    if (command == string("create_fourth_quarter")){
        EdgarData eData = EdgarData();
        O_Stock stock = findStockByTicker( argv[2] );
        size_t year = stoi( argv[3]);
        eData.createFourthQuarter( stock, year);
    }
    if (command == string("create_ttm_eps")){
        EdgarData eData = EdgarData();
        O_Stock stock = findStockByTicker( argv[2] );
        eData.createTtmEps(stock);
    }

// should be passed a ticker?
    if (command == string("get_quarters")){
        EdgarData eData = EdgarData();
//        string ticker("IBM");
        O_Stock stock = findStockByTicker( argv[2] );
        eData.getQuarters( stock );
    }
    if (command == string("get_single_quarter")){
        EdgarData eData = EdgarData();

        O_Stock stock = findStockByTicker( argv[2] );
        string acn = argv[3];
        eData.getSingleQarter( stock, acn );
    }

    if (command == string("getfyed")){
       // Test test;
       // test.setTestDB();

        EdgarData eData = EdgarData();

        std::vector<O_Stock> stocks;

        if (argc > 2)
        {
            auto stock = findStockByTicker( argv[2] );

            eData.getFiscalYearEndDate(stock);
            goto exit;
        }else{

            T_Stock ts;
            stocks = ts.select();


            path pp = basePath / string("no_yefd.txt");
            boost::filesystem::ofstream outFile(pp);

            for( auto it = stocks.begin(); it != stocks.end();++it)
            {
                if( it->_fiscal_year_end().length() == string("12-31").length())
                {
                    LOG_INFO << "Skipping for "<<it->_ticker()<<"\n";
                    continue;
                }
                if (!eData.getFiscalYearEndDate( *it ))
                {
                    outFile << "\"" << it->_ticker() <<"\",";
                    outFile.flush();
                }else
                    cout << "Succeeded to get fyed for"<< it->_ticker()<<"!\n";
            }
            outFile.close();
        }

    }

    if (command == string("copydevtotest")){
        EdgarData eData = EdgarData();
        T_Stock ts;
        auto stocks = ts.select();

        Test test;
        test.seedStocks(stocks);
    }

    if (command == string("getcountry")){
        EdgarData eData = EdgarData();
        eData.loadCountryMaps();
        T_Stock ts;
        auto stocks = ts.select();
        for(auto it = stocks.begin(); it != stocks.end(); ++it)
            eData.getCountry(*it);
    }

    if (command == string("")){
        showHelpMessage(argv[0]);
        LOG_ERROR << "unknown command: " << command;
    }

  exit:

    LOG_INFO << "deleting Config";
    delete config;
    
    LOG_INFO << "Last log message: deleting Logger";
    delete logger;

    return 0;
} 

int
main(int argc, char* argv[])
{
    try{
        return mainMain(argc, argv);
    }
    catch(exception& e){
        cout << e.what();
        flush(cout);
        LOG_ERROR << e.what();
    }
    return -1;
}
