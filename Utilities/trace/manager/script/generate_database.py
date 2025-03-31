#
#******************************************************************************
# @file    main.py
# @author  MCD team
# @brief   trace manager database generation
#*****************************************************************************
# @attention
#
# <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
# All rights reserved.</center></h2>
#
# This software component is licensed by ST under BSD 3-Clause license,
# the "License"; You may not use this file except in compliance with the
# License. You may obtain a copy of the License at:
#                        opensource.org/licenses/BSD-3-Clause
#
#*****************************************************************************
#
#!/usr/bin/env python

from reprlib import recursive_repr
import stringid_parser, data_base, argparse, glob, os


def askUserAction():
    return False

def main(args):
    # Open the database 
    DataBase = data_base.DB_IDstring()

    #print(args)
    print("")
    print("*********************************")
    print("")
    print("DBG : data file generation v0.1.0")
    print("")
    print("*********************************")
    print("")

    print(args)
    print("DBG: open the database")
    if (args.o != None):
        status = DataBase.open_database(args.o)
    else:
        status = DataBase.open_database()

    if (args.r == True):
        print("DBG: reset the database")
        status = DataBase.reset() 

    if False == status :
        print("DBG: erreur to find/create the database")
        return False

    if (args.r == True):
        print("DBG: reset the IDString DB")
        status = DataBase.reset()
    
    if (args.i != None):
        if os.path.isfile(args.i):
            StringID_files = [ args.i ]
        elif os.path.isdir(args.i):
            StringID_files = glob.glob(args.i + "**/*_idstring.h", recursive = True)
        else:
            print("DBG: invalid input argument <" + args.i +">")
            return 
    else:
        path = "./"
        StringID_files = glob.glob("./**/*_idstring.h", recursive = True)

    #print(StringID_files)

    for file in StringID_files:

        # Load the ID string file 
        status, app, dico = stringid_parser.parse_idstringfile(file)

        if status == True:

            # check if the data base already contains the application
            status, version = DataBase.GetApplicationInfo(app)

            if status == False:
                print("DATABASE:: the GUID " + app + " is not present inside the database")
            else:
                print("DATABASE:: the GUID " + app + " is already present inside the database")
                # compare the version
                print("DATABASE:: version[" + version + "] new version[" + dico['appVersion'] + "]")

            if (status == False):
                print("DATABASE:: Add the application <"+ app + ">")
                DataBase.AddApplicationInfo(app, dico)
            else:
                if (args.f == True) or (askUserAction() == True):
                    DataBase.AddApplicationInfo(app, dico)
                    print("DATABASE:: force the rewrite of the application [" + app + "]")
                else:
                    print("DATABASE:: cancel add the application [" + app + "]")
                    
    DataBase.save_database()
    print("")
    print("DBG::save the database <"+ DataBase.filename + ">")
    print("")

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "-input", help="Path (ie all the files with a naming xxxx_idstring.h) or file to get application IDStrings value as input value")
    parser.add_argument("-o", "-output", help="specify the json file to store all the application IDstring information, by default create where the script is called a file IDString_DB.json")
    parser.add_argument("-f", "-force", action='store_true', help="force the write of application IDString into the database, the new inputs are prior to any information already present in the database")
    parser.add_argument("-r", "-reset", action='store_true', help="force the database reset")
    args = parser.parse_args()

    main(args)


