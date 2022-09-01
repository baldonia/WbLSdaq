/**
 *  Copyright 2014 by Benjamin Land (a.k.a. BenLand100)
 *
 *  WbLSdaq is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  WbLSdaq is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with WbLSdaq. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <pthread.h>
#include <iostream>
#include <map>
#include <vector>
#include <stdexcept>
#include <unistd.h>
#include <ctime>
#include <cmath>
#include <csignal>
#include <cstring>
#include <fstream>

#include "RunDB.hh"
#include "BoardCommManager.hh"
#include "CONETNetwork.hh"
#include "VMEBridge.hh"
#include "V1730_wavedump.hh"
#include "V1730_dummy.hh"
#include "V1730_dpppsd.hh"
#include "V1730.hh"
#include "V1742.hh"
#include "V65XX.hh"
#include "LeCroy6Zi.hh"
#include "EthernetCommunication.hh"
#include "FileCommunication.hh"
#include "RunType.hh"
#include "Decode.hh"
#include "Readout.hh"

using namespace std;
using namespace H5;

int main(int argc, char **argv) {
    if (argc < 2) {
        cout << "usage: ./WbLSdaq FILE..." << endl;
        return -1;
    }

    for (int i = 1 ; i < argc ; i++){
	    stop = false;
	    readout_running = decode_running = false;
	    signal(SIGINT,int_handler);

	    cout << "Reading configuration..." << endl;
	    
	    RunDB db;
	    db.addFile(argv[i]);
	    RunTable run = db.getTable("RUN");
	    
	    readout(db, run);
    }
    return 0;
}
