#include <coopy/Stat.h>

//using namespace std;
//using namespace coopy::store;

/*
Stat Stat::normalize(const FloatSheet& sheet, 
		     int first, int last, float sc, bool modify) {
  Stat s;
  int hh = sheet.height();
  int ww = sheet.width();
  if (first==-1) first = 0;
  if (last==-1) last = ww-1;
  float tot = 0;
  float tot2 = 0;
  int ct = 0;
  for (int i=0; i<hh; i++) {
    for (int j=first; j<=last; j++) {
      float r = sheet.cell(j,i);
      tot += r;
      tot2 += r*r;
      ct++;
    }
  }
  if (ct==0) return s;
  float mean = tot;
  float dev = 1;
  mean /= ct;
  dev = tot2 / ct - mean*mean;
  if (dev<0) { 
    dev = 0; 
  } else {
    dev = sqrt(dev);
  }
  //printf("mean %g, dev %g\n", mean, dev);
  if (dev<sc) dev = sc;
  if (modify) {
    for (int i=0; i<hh; i++) {
      for (int j=first; j<=last; j++) {
	float r = sheet.cell(j,i);
	r = (r-mean)/dev;
	sheet.cell(j,i) = r;
      }
    }
  }
  s.mean = mean;
  s.stddev = dev;
  s.valid = true;
  return s;
}

*/
