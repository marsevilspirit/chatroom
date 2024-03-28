#include <iostream>
#include <vector>
#include "json.hpp"
using json_t = nlohmann::json;
int main() {
  json_t j;                                     // json对象
  // 添加json对象
  j["age"] = 21;                                // "age":23
  j["name"] = "diyuyi";                         // "name":diyuyi
  j["gear"]["suits"] = "2099";                  // "gear":{"suits":"2099"}
  j["jobs"] = {"student"};                      // "jobs":["student"]
  std::vector<int> v = {1, 2, 3};
  j["numbers"] = v;                             // "numbers":[1, 2, 3]
  std::map<std::string, int> m = {{"one", 1},{"two", 2}};
  j["map"] = m;                                 // "map":{"one", 1},{"two", 2}

  // 序列化得到json文本形式
  std::cout << j.dump() << std::endl;           // 无缩进
  std::cout << j.dump(2) << std::endl;   // 缩进两格
}
