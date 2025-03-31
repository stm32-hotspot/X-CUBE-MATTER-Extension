
from multiprocessing.sharedctypes import Value
from pycparser import parse_file, c_ast, c_generator
import sys, os, re, pprint 

def comment_remover(text):
    def replacer(match):
        s = match.group(0)
        if s.startswith('/'):
            return ""
        else:
            return s
    pattern = re.compile(
        r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
        re.DOTALL | re.MULTILINE
    )
    return re.sub(pattern, replacer, text)

#
# This function get the comment associated with a variable defintion
#
def get_comment(filename,str):
    comment = str
    fin = open(filename, "r")
    lines = fin.readlines()
    for line in lines:
        pos = line.find(str)
        if pos != -1:
            # print(line)
            # print(pos)
            # print(len(str))
            carac = line[pos + len(str)]
            if carac.isalpha() == False and carac.isalnum() == False:
                # extrat the comment /* "comment" */
                start = line.find("/*!<")
                end   = line.find("*/")
                comment = line[start+4:end].strip()
    fin.close()
    # print("get_comment::<" + comment + ">")
    return comment

#
# This function generates a clean header file to prepare the pycparsing
#
def generate_clean_file(filename):
    t = "typedef unsigned int uint32_t; \n\r" \
        "typedef unsigned short uint16_t;\n\r"\
        "typedef uint32_t UTIL_TRACE_MNG_AppTypeDef;\n\r"\
        "typedef uint16_t UTIL_TRACE_MNG_VersionTypeDef;\n\r"
    try:
        f_in = open(filename, "r")
        f_out = open("tmp.h", "w")
    except:
        print("file not found:: " + filename)
        return False
           
    lines = f_in.readlines()
    for line in lines:
        f = False
        if line.find("#include") != -1:
            f = True
        if line.find("define") != -1:
            f = True
        if line.find("ifndef") != -1:
            f = True
        if line.find("endif") != -1:
            f = True
        if line.find("ifdef") != -1:
            f = True
        if line.find("extern") != -1:
            f = True
        if line.find("};") == -1 and line.find("}") != -1:
            f = True
        if f == False:
            t = t + line
    f_out.write(comment_remover(t))
    f_in.close()
    f_out.close()
    return True


#
# This function parse an idstring file and return it's associated dictionnary
#
def parse_idstringfile(filename):

    print()
    print("ANALYSE : [" + filename + "]")
    
    # parse the file with comment
    if generate_clean_file(filename) == False:
        return false, "", {}

    ast = parse_file("tmp.h", use_cpp = False)
    os.remove("tmp.h")
    # get the information from the cleaned file
    b_GUID = False
    b_VERSION = False
    b_ENUM = False
    index = 1
    dico = {}

    for elt in ast.ext:

        k_GUID = ""        
        k_VERSION = ""
        k_ENUM = ""
        try :
            k_GUID = elt.type.type.type.names[0]
        except:
            #  nothing to do the field doesn't exist
            K_GUID = "ERROR"

        try :
            k_VERSION = elt.type.type.names[0]
        except:
            #  nothing to do the field doesn't exist
            k_VERSION = "ERROR"

        try :
            k_ENUM = elt.type.name
        except:
            #  nothing to do the field doesn't exist
            k_ENUM = "ERROR"
                    

        # get application GUID
        if k_GUID == 'UTIL_TRACE_MNG_AppTypeDef':
            app_id      = elt.name
            app_val     = elt.init.exprs[0].value + elt.init.exprs[1].value + elt.init.exprs[2].value + elt.init.exprs[3].value
            app_val     = app_val.replace('0x', '')
            app_comment = get_comment(filename, app_id)
            dico["appName"] = app_comment
            b_GUID = True

        # get application version
        if k_VERSION == 'UTIL_TRACE_MNG_VersionTypeDef':
            dico["appVersion"] = elt.init.value
            b_VERSION = True

        # get the enumeration of stringID
        if k_ENUM.find("IDSTRING") != -1:
            b_dico_init = False
            
            for data in elt.type.values.enumerators:
                # print(data.name)
                e_name = get_comment(filename, data.name)
                e_val = data.value
                if e_val == None:
                    e_val = index
                else:
                    e_val = int(data.value.value)
                    index = e_val
                index = index + 1    
                if b_dico_init == False:
                    #dico["IDStrings"] = { str(hex(e_val)).replace("0x","") : e_name }
                    dico["IDStrings"] = { int(e_val): e_name }
                    b_dico_init = True
                else:
                    #dico["IDStrings"][str(hex(e_val)).replace("0x","")] = e_name
                    dico["IDStrings"][int(e_val)] = e_name
                # print("ENUM::", e_name,"::", e_val)

            b_ENUM = True
       

    if b_GUID == True and b_ENUM == True and b_VERSION == True:
        print("PARSING_STATUS:: " + filename + " is a valid file")
        status = True
    else:
        print("PARSING_STATUS:: " + filename + " is a invalid file")
        status = False

    # question : do we need to have a coherence between the data
    #print()
    #print("application:<" + app_val + ">")
    #pp = pprint.PrettyPrinter(indent=4)
    #pp.pprint(dico)

    return status, app_val, dico
    # find GUID
