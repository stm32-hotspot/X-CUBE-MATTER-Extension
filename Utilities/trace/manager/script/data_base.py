
import json

class DB_IDstring:
    "management of the StringID data base"

    def __init__(self):
        self.opened = False
        self.IDString_Dico = {"dictionaryVersion" : "1.0"}
        self.filename = "IDString_DB.json"

    def open_database(self, filename = "IDString_DB.json"):
        self.filename = filename
        try :
            print("open")
            database = open(self.filename, "r")
            self.IDString_Dico = json.load(database);
            print("close")
            database.close();
            print(self.IDString_Dico)
            self.opened = True
        except:
            self.opened = True
        
        return self.opened

    def reset(self):
        if self.opened == True:
            self.IDString_Dico = {"dictionnaryVersion" : "1.0"}

    def GetApplicationInfo(self, AppGUID):
        # temporary no check on the database
        AppVersion = 0
        Status = True
        try:
            AppVersion = self.IDString_Dico[AppGUID]["appVersion"]
        except:
            Status = False
        return Status, AppVersion

    def AddApplicationInfo(self, app, dico):
        # nnn
        self.IDString_Dico[app] = dico
        return True
    

    def save_database(self):
        database = open(self.filename, "w")
        json.dump(self.IDString_Dico, database, indent = 4, sort_keys = True)
        database.close();
        return True

