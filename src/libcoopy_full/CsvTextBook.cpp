
#include <coopy/CsvTextBook.h>
#include <coopy/CsvFile.h>
#include <coopy/FormatSniffer.h>

#include <algorithm>

using namespace coopy::store;
using namespace coopy::format;
using namespace std;

static string getRoot(const char *fname) {
  string root = fname;
  if (root.rfind("/")!=string::npos) {
    root = root.substr(0,root.rfind("/")+1);
  } else if (root.rfind("\\")!=string::npos) {
    root = root.substr(0,root.rfind("\\")+1);
  } else {
    root = "";
  }
  return root;
}

bool CsvTextBook::readCsvsData(const char *data, int len) {
  if (!compact) return false;
  clear();
  Property p;
  if (CsvFile::read(data,len,*this,p)!=0) {
    fprintf(stderr,"Failed to read CSVS data\n");
    return false;
  }
  for (int i=0; i<(int)sheets.size(); i++) {
    sheets[i].setRowOffset();
  }
  return true;
}


std::string CsvTextBook::writeCsvsData() {
  string result;
  write(NULL,this,true,&result);
  return result;
}

bool CsvTextBook::readCsvs(const char *fname) {
  if (compact) {
    clear();
    Property p;
    if (CsvFile::read(fname,*this,p)!=0) {
      fprintf(stderr,"Failed to read %s\n", fname);
      return false;
    }
    for (int i=0; i<(int)sheets.size(); i++) {
      sheets[i].setRowOffset();
    }
    dbg_printf("Read CSVS file %s\n", fname);
    return true;
  }

  CsvSheet index;
  if (CsvFile::read(fname,index)!=0) {
    fprintf(stderr,"Failed to read %s\n", fname);
    return false;
  }
  string root = getRoot(fname);
  for (int y=0; y<index.height(); y++) {
    string cmd = index.cell(0,y);
    if (cmd=="table") {
      string key = index.cell(1,y);
      //printf("key %s\n", key.c_str());
      string f = root + key + ".csv";
      dbg_printf("Adding %s [%s]\n", key.c_str(), f.c_str());
      name2index[key] = (int)sheets.size();
      CsvSheet *data = new CsvSheet;
      if (data==NULL) {
	fprintf(stderr,"Failed to allocated data sheet\n");
	return false;
      }
      if (CsvFile::read(f.c_str(),*data)!=0) {
	fprintf(stderr,"Failed to read %s referenced from %s\n", f.c_str(),
		fname);
	delete data;
	return false;
      }
      PolySheet sheet(data,true);
      sheets.push_back(sheet);
      names.push_back(key);
    }
  }
  return true;
}

bool CsvTextBook::write(const char *fname, TextBook *book, bool compact,
			std::string *output) {
  if (fname==NULL && output==NULL) return false;
  vector<string> names = book->getNames();
  if (compact) {
    Property p;
    if (fname) {
      p.put("file",fname);
    }
    int len = (int)names.size();
    for (int i=0; i<len; i++) {
      if (book->namedSheets() || len>1) {
	FILE *fp = NULL;
	if (fname) {
	  if (string(fname)=="-") {
	    fp = stdout;
	  } else {
	    fp = fopen(fname,(i>0)?"ab":"wb");
	    if (!fp) {
	      fprintf(stderr,"CsvTextBook: could not open %s\n", fname);
	      return false;
	    }
	  }
	}
	if (i>0) {
	  // use Windows encoding, since UNIX is more forgiving
	  string eol = " \r\n";
	  if (fp) {
	    fprintf(fp,"%s",eol.c_str());
	  } else {
	    *output += eol;
	  }
	}
	// use Windows encoding, since UNIX is more forgiving
	if (fp) {
	  fprintf(fp,"== %s ==\r\n", names[i].c_str());
	} else {
	  *output += "== ";
	  *output += names[i];
	  *output += " ==\r\n";
	}
	if (fp) {
	  if (fp!=stdout) {
	    fclose(fp);
	    fp = NULL;
	  }
	}
	p.put("append",true);
	p.put("mark_header",true);
      }
      PolySheet sheet = book->readSheetByIndex(i);
      dbg_printf("  writing CSVS sheet %s\n", names[i].c_str());
      if (!output) {
	CsvFile::write(sheet,p);
      } else {
	*output += CsvFile::writeString(sheet,p);
      }
    }
    return true;
  }

  string root = getRoot(fname);
  CsvSheet idx;
  bool ok = true;
  for (int i=0; i<(int)names.size(); i++) {
    idx.addField("table",false);
    idx.addField(names[i].c_str(),false);
    idx.addField("",false);
    idx.addRecord();
    string f = root + names[i] + ".csv";
    //printf("%s\n", f.c_str());
    CsvFile::write(book->readSheetByIndex(i),f.c_str());
  }
  ok = ok && (CsvFile::write(idx,fname)==0);
  return ok;
}

//bool CsvTextBook::open(const Property& config) {
//  if (!config.check("file")) return false;
//  return readCsvs(config.get("file").asString().c_str());
//}

bool CsvTextBook::addSheet(const SheetSchema& schema) {
  dbg_printf("csvtextbook::addsheet %s\n", schema.getSheetName().c_str());
  string name = schema.getSheetName();
  if (!schema.hasSheetName()) {
    named = false;
  }
  if (find(names.begin(),names.end(),name)!=names.end()) {
    return false;
  }
  CsvSheet *data = new CsvSheet;
  if (data==NULL) {
    fprintf(stderr,"Failed to allocated data sheet\n");
    return false;
  }
  if (schema.hasSheetName()) {
    data->setSheetName(name.c_str());
  }
  PolySheet sheet(data,true);
  name2index[name] = (int)sheets.size();
  sheets.push_back(sheet);
  names.push_back(name);
  data->setWidth(schema.getColumnCount());

  SimpleSheetSchema *rec = new SimpleSheetSchema;
  COOPY_ASSERT(rec);
  rec->copy(schema);
  sheets.back().setSchema(rec,true);

  //COOPY_ASSERT(rec);
  //rec->copy(schema);
  //data->setSchema(Poly<SheetSchema>(rec,true));

  /*
  CsvSheetSchema *rec = new CsvSheetSchema(data,name,0);
  COOPY_ASSERT(rec);
  data->setSchema(Poly<SheetSchema>(rec,true));
  */
  return true;
}


CsvSheet *CsvTextBook::nextSheet(const char *name, bool named) {
  CsvSheet *data = new CsvSheet;
  if (data==NULL) {
    fprintf(stderr,"Failed to allocated data sheet\n");
    return NULL;
  }
  PolySheet sheet(data,true);
  name2index[name] = (int)sheets.size();
  sheets.push_back(sheet);
  names.push_back(name);
  this->named = named;
  dbg_printf("Adding CSV sheet %s (named? %d)\n", name, named);
  return data;
}

