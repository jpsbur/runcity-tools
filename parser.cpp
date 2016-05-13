#include <cstdio>
#include <expat.h>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <cmath>

using namespace std;

struct Node {
  long long id;
  double lon, lat;

  Node();
  Node(map <string, string> &mattr);
};
struct Way {
  long long id;
  vector <long long> nodes;
  string name;

  Way();
  Way(map <string, string> &mattr);
  double length();
};

map <long long, Node> nodes;
map <long long, Way> ways;
Way curWay;

Node::Node() {
  id = -1;
}
Node::Node(map <string, string> &mattr) {
  sscanf(mattr["id"].c_str(), "%lld", &id);
  sscanf(mattr["lon"].c_str(), "%lf", &lon);
  sscanf(mattr["lat"].c_str(), "%lf", &lat);
  lon *= M_PI / 180.0;
  lat *= M_PI / 180.0;
}

Way::Way() {
  id = -1;
}
Way::Way(map <string, string> &mattr) {
  sscanf(mattr["id"].c_str(), "%lld", &id);
}
double dist(const Node &n1, const Node &n2) {
  static double R = 6371000.0;
  double phi1 = n1.lat;
  double phi2 = n2.lat;
  double dphi = phi2 - phi1;
  double dlam = n2.lon - n1.lon;
  double sp2 = sin(dphi * 0.5);
  double sl2 = sin(dlam * 0.5);
  double a = sp2 * sp2 + cos(phi1) * cos(phi2) * sl2 * sl2;
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}
double Way::length() {
  double res = 0.0;
  for (int i = 1; i < (int)nodes.size(); ++i) {
    res += dist(::nodes[nodes[i - 1]], ::nodes[nodes[i]]);
  }
  return res;
}

void elemStart(void *data, const char *el, const char **attr) {
  map <string, string> mattr;
  for (int i = 0; attr[i]; i += 2) {
    mattr[attr[i]] = attr[i + 1];
  }
  if (!strncmp(el, "node", 4)) {
    Node n(mattr);
    nodes[n.id] = n;
  } else if (!strncmp(el, "way", 3)) {
    curWay = Way(mattr);
  } else if (!strncmp(el, "nd", 2)) {
    if (curWay.id == -1) {
      return;
    }
    long long id;
    sscanf(mattr["ref"].c_str(), "%lld", &id);
    curWay.nodes.push_back(id);
  } else if (!strncmp(el, "tag", 3)) {
    if (curWay.id == -1) {
      return;
    }
    if (mattr["k"] == string("name")) {
      curWay.name = mattr["v"];
    }
  }
}

void elemEnd(void *data, const char *el) {
  if (!strncmp(el, "way", 3)) {
    ways[curWay.id] = curWay;
    curWay = Way();
  }
}

const int BUF_LEN = (1 << 16);
char buf[BUF_LEN];

int main() {
  XML_Parser p = XML_ParserCreate(NULL);
  XML_SetElementHandler(p, elemStart, elemEnd);
  while (true) {
    int done;
    int len;

    len = fread(buf, 1, BUF_LEN, stdin);
    if (len < 0) {
      fprintf(stderr, "Read error\n");
      exit(-1);
    }
    done = feof(stdin);

    if (!XML_Parse(p, buf, len, done)) {
      fprintf(stderr, "Parse error at line %d:\n%s\n",
        (int)XML_GetCurrentLineNumber(p),
        XML_ErrorString(XML_GetErrorCode(p)));
      exit(-1);
    }
    if (done) {
      break;
    }
  }
  fprintf(stderr, "Nodes: %d\n", (int)nodes.size());
  fprintf(stderr, "Ways: %d\n", (int)ways.size());
  vector <string> wayTypes;
  wayTypes.push_back("улица");
  wayTypes.push_back("проспект");
  wayTypes.push_back("шоссе");
  wayTypes.push_back("проезд");
  wayTypes.push_back("переулок");
  wayTypes.push_back("площадь");
  wayTypes.push_back("пруд");
  wayTypes.push_back("парк");
  map <string, map <string, vector <long long> > > waysByType;
  for (auto w: ways) {
    if (w.second.name != "") {
      //fprintf(stderr, "way: %s (%lf meters, %lld .. %lld)\n", w.second.name.c_str(), w.second.length(), *w.second.nodes.begin(), *w.second.nodes.rbegin());
      for (auto wt: wayTypes) {
        if (strstr(w.second.name.c_str(), wt.c_str())) {
          waysByType[wt][w.second.name].push_back(w.first);
        }
      }
    }
  }
  for (auto wt: wayTypes) {
    fprintf(stderr, "%s: %d\n", wt.c_str(), (int)waysByType[wt].size());
    if (waysByType[wt].size() < 50) {
      for (auto w: waysByType[wt]) {
        fprintf(stderr, "  %s\n", w.first.c_str());
      }
    }
  }
  return 0;
}
