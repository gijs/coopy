#include <stdio.h>
#include <getopt.h>

#include <coopy/PolyBook.h>
#include <coopy/NameSniffer.h>
#include <coopy/IndexSniffer.h>
#include <coopy/ShortTextBook.h>
#include <coopy/CsvTextBook.h>
#include <coopy/Dbg.h>
#include <coopy/CompareFlags.h>
#include <coopy/Options.h>
#include <coopy/PoolImpl.h>
#include <coopy/PatchParser.h>
#include <coopy/MergeOutputPool.h>
#include <coopy/Highlighter.h>
#include <coopy/Diff.h>

using namespace coopy::store;
using namespace coopy::cmp;
using namespace coopy::app;
using namespace std;

int main(int argc, char *argv[]) {
  Options opt("ssformat");
  int r = opt.apply(argc,argv);
  if (r!=0) return r;

  bool help = opt.checkBool("help");
  const vector<string>& core = opt.getCore();
  if ((core.size()<1 && (opt.checkString("mode")=="")) || help) {
    opt.beginHelp();
    opt.addUsage("ssformat [options] FILE");
    opt.addUsage("ssformat [options] FILE1 FILE2");
    opt.addDescription("Reformat tables/databases/spreadsheets.");
    opt.showOptions(OPTION_FOR_FORMAT);
    opt.addExample("ssformat numbers.csv numbers_converted.sqlite",
		   "Convert CSV format table to an Sqlite database table.").require("numbers.csv");
    opt.addExample("ssformat numbers.sqlite numbers_converted.csv",
		   "Convert Sqlite database table to a CSV format table.").require("numbers.sqlite");
    opt.addExample("ssformat numbers.sqlite -",
		   "Display contents of an Sqlite database table.").require("numbers.sqlite");
    opt.endHelp();
    return help?0:1;
  }

  Diff diff;
  return diff.apply(opt);

  /*

    // old implementation of ssformat
    // delete when sure nothing of importance was left unreimplemented

  Options opt("ssformat");
  int r = opt.apply(argc,argv);
  if (r!=0) return r;

  bool extractHeader = opt.checkBool("header");
  bool omitHeader = opt.checkBool("omit-header");
  bool extractIndex = opt.checkBool("index");
  bool verbose = opt.checkBool("verbose");
  bool help = opt.checkBool("help");
  string inputFormat = opt.checkString("input-format");
  string outputFormat = opt.checkString("output-format");
  const vector<string>& include_columns = opt.getCompareFlags().include_columns;
  const vector<string>& exclude_columns = opt.getCompareFlags().exclude_columns;
  bool have_includes = (include_columns.size() > 0);
  bool have_excludes = (exclude_columns.size() > 0);
  string sheetSelection = "";

  if (opt.getCompareFlags().tables.size()>1) {
    fprintf(stderr,"sorry, can only select one table right now\n");
    return 1;
  }
  if (opt.getCompareFlags().tables.size()==1) {
    sheetSelection = *(opt.getCompareFlags().tables.begin());
  }

  const vector<string>& core = opt.getCore();

  if (core.size()>2) {
    fprintf(stderr, "Options not understood\n");
    return 1;
  }

  if (core.size()<1||help) {
    opt.beginHelp();
    opt.addUsage("ssformat [options] FILE");
    opt.addUsage("ssformat [options] FILE1 FILE2");
    opt.addDescription("Reformat tables/databases/spreadsheets.");
    opt.showOptions(OPTION_FOR_FORMAT);
    opt.addExample("ssformat numbers.csv numbers_converted.sqlite",
		   "Convert CSV format table to an Sqlite database table.").require("numbers.csv");
    opt.addExample("ssformat numbers.sqlite numbers_converted.csv",
		   "Convert Sqlite database table to a CSV format table.").require("numbers.sqlite");
    opt.addExample("ssformat numbers.sqlite -",
		   "Display contents of an Sqlite database table.").require("numbers.sqlite");
    opt.endHelp();
    return help?0:1;
  }

  string out_file = "-";
  if (core.size()==2) {
    out_file = core[1];
  }

  PolyBook src;
  if (!src.readAndWillWrite(core[0].c_str(),inputFormat.c_str(),
			    out_file.c_str(),outputFormat.c_str())) {
    fprintf(stderr,"Failed to read %s\n", core[0].c_str());
    return 1;
  }

  CompareFlags mflags;
  std::string meta_file = opt.checkString("meta");
  PoolImpl pool;
  mflags.pool = &pool;
  if (meta_file!="") {
    MergeOutputPool pooler;
    pooler.attachBook(src);
    pooler.startOutput("-",mflags);
    pooler.setFlags(mflags);
    PatchParser parser(&pooler,meta_file,mflags);
    bool ok = parser.apply();
    pooler.stopOutput("-",mflags);
    if (!ok) {
      fprintf(stderr,"Failed to read %s\n", meta_file.c_str());
      return 1;
    }
  }

  if (sheetSelection!="") {
    CsvTextBook *book = new CsvTextBook(true);
    if (book==NULL) {
      fprintf(stderr,"Failed to allocate output\n");
      return 1;
    }
    Property p;
    p.put("sheet",sheetSelection.c_str());
    book->copy(src,p);
    src.take(book);

    //vector<string> names = book->getNames();
    //printf("have %d\n", names.size());
  }
  if (extractHeader) {
    PolySheet sheet = src.readSheetByIndex(0);
    CompareFlags flags;
    NameSniffer sniff(sheet,flags);
    ShortTextBook *book = new ShortTextBook();
    if (book==NULL) {
      fprintf(stderr,"Failed to allocate output\n");
      return 1;
    }
    for (int i=0; i<sheet.width(); i++) {
      book->sheet.addField(sniff.suggestColumnName(i).c_str(),false);
    }
    book->sheet.addRecord();
    src.take(book);
  }
  if (extractIndex) {
    PolySheet sheet = src.readSheetByIndex(0);
    CompareFlags flags;
    NameSniffer nsniff(sheet,flags);
    IndexSniffer sniff(sheet,flags,nsniff);
    vector<int> indexes = sniff.suggestIndexes();
    dbg_printf("Index count %d\n", (int)indexes.size());
    ShortTextBook *book = new ShortTextBook();
    if (book==NULL) {
      fprintf(stderr,"Failed to allocate output\n");
      return 1;
    }
    book->sheet.copy(sheet);
    int at = 0;
    for (int i=0; i<sheet.width(); i++) {
      if (indexes[i]==0) {
	book->sheet.deleteColumn(at);
      } else {
	at++;
      }
    }
    src.take(book);
  }
  
  src.setPool(&pool);
    
  if (have_includes||have_excludes) {
    printf("INPLACE? %d\n", src.inplace());

    map<string,int> includes;
    map<string,int> excludes;
    for (int i=0; i<(int)include_columns.size(); i++) { 
      includes[include_columns[i]] = 1;
    }
    for (int i=0; i<(int)exclude_columns.size(); i++) { 
      excludes[exclude_columns[i]] = 1;
    }
    for (int i=0; i<src.getSheetCount(); i++) {
      PolySheet sheet = src.readSheetByIndex(i);
      CompareFlags flags;
      NameSniffer sniff(sheet,flags);
      const std::vector<std::string>& names = sniff.suggestNames();
      int at = 0;
      for (int j=0; j<(int)names.size(); j++) {
	string name = names[j];
	bool included = includes.find(name) != includes.end();
	bool excluded = excludes.find(name) != excludes.end();
	if (have_excludes && excluded) {
	  ColumnRef col(at);
	  sheet.deleteColumn(at);
	  continue;
	}
	if (have_includes && !included) {
	  ColumnRef col(at);
	  sheet.deleteColumn(at);
	  continue;
	}
	at++;
      }
    }
  }

  if (omitHeader) {
    for (int i=0; i<src.getSheetCount(); i++) {
      PolySheet sheet = src.readSheetByIndex(i);
      CompareFlags flags;
      NameSniffer sniff(sheet,flags);
      if (sniff.isEmbedded()) {
	RowRef row0(0);
	RowRef rowh(sniff.getHeaderHeight()-1);
	sheet.deleteRows(row0,rowh);
      }
    }
  }

  if (opt.checkBool("paint")) {
    if (!src.attach(out_file.c_str())) {
      fprintf(stderr,"Failed to attach %s\n", out_file.c_str());
      return 1;
    }
    Highlighter h;
    h.apply(src);
    src.flush();
  } else {
    if (!src.write(out_file.c_str(),outputFormat.c_str())) {
      fprintf(stderr,"Failed to write %s\n", out_file.c_str());
      return 1;
    }
  }

  return 0;
  */
}
