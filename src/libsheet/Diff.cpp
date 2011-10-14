#include <stdio.h>
#include <getopt.h>

#include <coopy/CsvFile.h>
#include <coopy/CsvTextBook.h>
#include <coopy/MergeOutputIndex.h>
#include <coopy/BookCompare.h>
#include <coopy/PolyBook.h>
#include <coopy/SheetPatcher.h>
#include <coopy/PatchParser.h>
#include <coopy/Options.h>

#include <coopy/Diff.h>

using namespace std;
using namespace coopy::app;
using namespace coopy::store;
using namespace coopy::cmp;

static Patcher *createTool(string mode, string version="") {
  return Patcher::createByName(mode.c_str(),version.c_str());
}

int Diff::apply(const Options& opt) {
  bool verbose = opt.checkBool("verbose");
  bool equality = opt.checkBool("equals");
  bool apply =  opt.checkBool("apply");
  std::string output = opt.checkString("output");
  std::string parent_file = opt.checkString("parent");
  std::string patch_file = opt.checkString("patch");
  std::string cmd = opt.checkString("cmd");
  std::string version = opt.checkString("version");
  std::string mode = opt.checkString("mode",
				     opt.isDiffLike()?"tdiff":"merge");

  CompareFlags flags = opt.getCompareFlags();

  vector<string> core = opt.getCore();
  if (opt.isMergeLike()) {
    if (core.size()>0) {
      parent_file = core[0];
      core.erase(core.begin());
    }
  }
  if (opt.isPatchLike()) {
    if (core.size()>0) {
      patch_file = core.back();
      core.erase(core.begin()+core.size()-1);
    }
  }

  BookCompare cmp;
  cmp.setVerbose(verbose);

  PolyBook _pivot;
  PolyBook *pivot;
  PolyBook _local;
  PolyBook *local = &_local;
  PolyBook _remote;
  PolyBook *remote = &_remote;

  string local_file;
  if (core.size()>=1) {
    local_file = core[0];
  }
  
  string remote_file;
  if (core.size()>=2) {
    remote_file = core[1];
  }

  if (local_file!="") {
    if (!_local.read(local_file.c_str())) {
      fprintf(stderr,"Failed to read %s\n", local_file.c_str());
      return 1;
    }
    flags.local_uri = local_file;
  }

  if (remote_file!="") {
    if (!_remote.read(remote_file.c_str())) {
      fprintf(stderr,"Failed to read %s\n", remote_file.c_str());
      return 1;
    }
    flags.remote_uri = remote_file;
  }

  if (parent_file!="") {
    if (!_pivot.read(parent_file.c_str())) {
      fprintf(stderr,"Failed to read %s\n", parent_file.c_str());
      return 1;
    }
    flags.pivot_uri = parent_file;
    pivot = &_pivot;
  } else {
    pivot = &_local;
    flags.pivot_sides_with_local = true;
  }

  if (equality) {
    if (*local == *remote) {
      return 0;
    }
    return 1;
  }

  Patcher *diff = createTool(mode,version);
  if (diff==NULL) {
    fprintf(stderr,"Failed to create handler for patch mode: '%s'\n",
	    mode.c_str());
    return 1;
  }
  PolyBook obook;
  if (diff->needOutputBook()) {
    if (!obook.attach(output.c_str())) {
      delete diff; diff = NULL;
      return 1;
    }
    diff->attachOutputBook(obook);
  }
  PolyBook tbook;
  if (diff->outputStartsFromInput()) {
    if (output!="-") {
      if (!_local.write(output.c_str())) {
	delete diff; diff = NULL;
	return 1;
      }
      if (!_local.read(core[0].c_str())) {
	fprintf(stderr,"Failed to read %s\n", core[0].c_str());
	return 1;
      }
      if (!tbook.read(output.c_str())) {
	fprintf(stderr,"Failed to read %s\n", output.c_str());
	return 1;
      }
    } else {
      tbook.take(new CsvTextBook(true));
      Property p;
      tbook.copy(_local,p);
    }
    diff->attachBook(tbook);
  } else {
    diff->attachBook(*local);
  }

  if (apply) {
    SheetPatcher *apply_diff = SheetPatcher::createForApply();
    COOPY_ASSERT(apply_diff!=NULL);
    apply_diff->showSummary(diff);
    diff = apply_diff;
    diff->attachBook(*local);
  }
  if (!diff->startOutput(output,flags)) {
    fprintf(stderr,"Patch output failed\n");
    delete diff;
    diff = NULL;
    return 1;
  }
  if (patch_file==""&&cmd=="") {
    cmp.compare(*pivot,*local,*remote,*diff,flags);
  } else {
    diff->setFlags(flags);
    PatchParser parser(diff,patch_file,cmd);
    bool ok = parser.apply();
    if (!ok) {
      fprintf(stderr,"Patch failed\n");
    }
  }
  diff->stopOutput(output,flags);
  if (diff->needOutputBook()) {
    obook.flush();
  }
  if (diff->outputStartsFromInput()) {
    if (!tbook.write(output.c_str())) {
      fprintf(stderr,"Failed to write %s\n", output.c_str());
      return 1;
    }
  }
  if (apply) {
    if (diff->getChangeCount()>0) {
      if (!local->inplace()) {
	if (!local->write(core[0].c_str())) {
	  fprintf(stderr,"Failed to write %s\n", core[0].c_str());
	  return 1;
	}
      }
    }
  }
  delete diff;
  diff = NULL;

  return 0;
}

