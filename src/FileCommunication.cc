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

#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdexcept>
#include <FileCommunication.hh>

FileCommunication::FileCommunication(std::string path){
    fd = open(path.c_str(), O_RDWR);
    if (fd == -1){
        perror("error is: ");
        throw std::runtime_error("Could not open file " + std::to_string(errno));
    }
}
