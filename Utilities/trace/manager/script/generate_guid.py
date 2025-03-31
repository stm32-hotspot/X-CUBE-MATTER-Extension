import uuid, argparse, os



if __name__ == "__main__":

    print("")
    print("*********************************")
    print("")
    print("GETGUID : get guid v1.0.0")
    print("")
    print("*********************************")
    print("")

    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "-app",     help="specify the short name of the application")
    parser.add_argument("-c", "-comment", help="specify the comment display for the application inside the trace manager tool")
    parser.add_argument("-o", "-output", help="specify the output folder where to generate the header file")
    args = parser.parse_args()

    if (args.a == None):
        print("please used -a to specify the short name of the application")
        exit(-1)

    APP = args.a.replace(" ", "").upper()
    app = APP.lower()

    if (args.c != None):
        App_comment = args.c
    else:
        App_comment = APP

    if (args.o != None):
        App_folder = args.o
    else:
        App_folder = "./"

    try: 
        f = open(App_folder + app + "_idstring.h", "w")
    except:
        print("Erreur to create the header file of our application")
        exit(-2)

    # generate the header of the file
    f.write("/**\n")
    f.write("******************************************************************************\n")
    f.write("* @file    "+ app + "_idstring.h\n")
    f.write("* @author\n")
    f.write("* @brief   " + APP + " application id string definition\n")
    f.write("******************************************************************************\n")
    f.write("* @attention\n")
    f.write("*\n")
    f.write("* <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.\n")
    f.write("* All rights reserved.</center></h2>\n")
    f.write("*\n")
    f.write("* This software component is licensed by ST under BSD 3-Clause license,\n")
    f.write("* the \"License\"; You may not use this file except in compliance with the\n")
    f.write("* License. You may obtain a copy of the License at:\n")
    f.write("*                        opensource.org/licenses/BSD-3-Clause\n")
    f.write("*\n")
    f.write("******************************************************************************\n")
    f.write("*/\n")
    f.write("\n")
    # add the recursive prevention + C++  
    f.write("/* Define to prevent recursive inclusion -------------------------------------*/\n")
    f.write("#ifndef " + APP + "_IDSTRING_H\n")
    f.write("#define " + APP + "_IDSTRING_H\n")
    f.write("\n")
    f.write("#ifdef __cplusplus\n")
    f.write("extern \"C\" {\n")
    f.write("#endif\n")
    f.write("\n")
    
    f.write("/* Includes ------------------------------------------------------------------*/\n")
    f.write("#include \"stm32_trace_mng.h\"\n")
    f.write("/* Exported macro ------------------------------------------------------------*/\n")

    guid = uuid.uuid4().hex
    # generate the GUID
    f.write("/** @defgroup " + APP + "_Def_Exported Application "+ APP + " Id definition export\n")
    f.write("* @{\n")
    f.write("*/\n")
    f.write("\n")
    f.write("static const UTIL_TRACE_MNG_AppTypeDef " + APP + "_GUID[] = { 0x" + guid[:8] + 
            ",0x" + guid[8:16] + ",0x" + guid[16:24] +",0x" + guid[24:]+ " };   /*!< " + App_comment + " */\n")
    f.write("\n")
    
    # generate the version 
    f.write("static const UTIL_TRACE_MNG_VersionTypeDef " + APP + "_IDSTRING_VERSION  = 0x000u;" +
            "  /*!< id version 0x\"major`\"minor\"\"patch\" */\n")
    f.write("\n")
    
    # generate the enumeration file 
    f.write("enum " + APP + "_IDSTRING {\n")
    for index in range(11):
        f.write("\t" + APP + "_IDSTRING_" + str(index) + "=" + str(index) + ",\t\t/*!< string for ID =" + str(index) + " */\n")
    f.write("};\n")

    f.write("/**\n")
    f.write("* @}\n")
    f.write("*/\n")
    f.write("\n")
    f.write("#ifdef __cplusplus\n")
    f.write("}\n")
    f.write("#endif\n")
    f.write("#endif /* " + APP + "_IDSTRING_H */\n")
    
    f.close()

    # end with success
    print("success....... the file " + app + "_idstring.h is generated")