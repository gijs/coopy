#include <coopy/ColumnInfo.h>
#include <coopy/Dbg.h>

#include <stdio.h>
#include <ctype.h>

using namespace coopy::store;

bool ColumnType::setType(const std::string& name, 
			 const std::string& lang) {
  reset();
  src_name = name;
  src_lang = lang;
  std::string _name = name;
  for (int i=0; i<(int)_name.length(); i++) {
    _name[i] = tolower(_name[i]);
    if (_name[i]=='(') {
      _name = _name.substr(0,i);
      break;
    }
  }
  if (_name=="int"||_name=="integer"||_name=="tinyint"||_name=="byte"||_name=="smallint"||_name=="smallint unsigned") {
    family = COLUMN_FAMILY_INTEGER;
  } else if (_name=="text") {
    family = COLUMN_FAMILY_TEXT;
  } else if (_name == "long integer") {
    family = COLUMN_FAMILY_INTEGER;
  } else if (_name == "float" || _name=="single" || _name=="double" || _name == "real") {
    family = COLUMN_FAMILY_REAL;
  } else if (_name == "datetime (short)"||_name=="datetime"||_name=="date"||_name=="timestamp") {
    family = COLUMN_FAMILY_DATETIME;
  } else if (_name=="text"||_name=="varchar"||_name.substr(0,7)=="varchar"||_name=="longtext") {
    family = COLUMN_FAMILY_TEXT;
  } else if (_name=="memo"||_name=="hyperlink"||_name=="memo/hyperlink") {
    family = COLUMN_FAMILY_TEXT;
  } else if (_name=="boolean"||_name=="bool") {
    family = COLUMN_FAMILY_BOOLEAN;
  } else if (_name=="currency") {
    family = COLUMN_FAMILY_CURRENCY;
  } else if (_name=="blob"||_name=="clob") {
    family = COLUMN_FAMILY_BLOB;
  }
  if (family==COLUMN_FAMILY_NONE) {
    if (name!="") {
      fprintf(stderr,"Unrecognized type \'%s\'; update src/libcoopy_core/ColumnInfo.cpp\n", name.c_str());
      if (name=="INTEGER REFERENCE blob") {
	fprintf(stderr,"Seems like a typo? To be confirmed...\n");
      } else {
	if (coopy_is_strict()) {
	  exit(1);
	}
      }
    }
  }

  return family!=COLUMN_FAMILY_NONE;
}


std::string ColumnType::asSqlite(bool addPrimaryKey) const {
  std::string result = "";
  switch (family) {
  case COLUMN_FAMILY_INTEGER:
    result = "INTEGER";
    break;
  case COLUMN_FAMILY_REAL:
    result = "REAL";
    break;
  case COLUMN_FAMILY_TEXT:
    result = "TEXT";
    break;
  case COLUMN_FAMILY_DATETIME:
    result = "DATETIME"; // for documentation purposes only
    break;
  case COLUMN_FAMILY_BOOLEAN:
    result = "BOOLEAN"; // for documentation purposes only
    break;
  case COLUMN_FAMILY_BLOB:
    result = "BLOB"; // for documentation purposes only
    break;
  }
  bool pk = false;
  if (primaryKeySet&&addPrimaryKey) {
    if (primaryKey) {
      if (result!="") {
	result += " ";
      }
      result += "PRIMARY KEY";
      pk = true;
    }
  }
  if (!allowNull) {
    if (!pk) {
      if (result!="") {
	result += " ";
      }
      result += "NOT NULL";
    }
  }
  return result;
}


