"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[15337],{28453:(e,n,r)=>{r.d(n,{R:()=>a,x:()=>l});var t=r(96540);const s={},i=t.createContext(s);function a(e){const n=t.useContext(i);return t.useMemo((function(){return"function"==typeof e?e(n):{...n,...e}}),[n,e])}function l(e){let n;return n=e.disableParentContext?"function"==typeof e.components?e.components(s):e.components||s:a(e.components),t.createElement(i.Provider,{value:n},e.children)}},96368:(e,n,r)=>{r.r(n),r.d(n,{assets:()=>o,contentTitle:()=>a,default:()=>p,frontMatter:()=>i,metadata:()=>l,toc:()=>c});var t=r(74848),s=r(28453);const i={},a="C++\u5ba2\u6237\u7aef",l={id:"client-tools/cpp-client",title:"C++\u5ba2\u6237\u7aef",description:"\u6b64\u6587\u6863\u4e3b\u8981\u662fTuGraph C++ SDK\u7684\u4f7f\u7528\u8bf4\u660e\u3002",source:"@site/versions/version-4.5.1/zh-CN/source/7.client-tools/2.cpp-client.md",sourceDirName:"7.client-tools",slug:"/client-tools/cpp-client",permalink:"/tugraph-db/zh/4.5.1/client-tools/cpp-client",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:2,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"Python\u5ba2\u6237\u7aef",permalink:"/tugraph-db/zh/4.5.1/client-tools/python-client"},next:{title:"Java\u5ba2\u6237\u7aef",permalink:"/tugraph-db/zh/4.5.1/client-tools/java-client"}},o={},c=[{value:"1.\u6982\u8ff0",id:"1\u6982\u8ff0",level:2},{value:"2.\u4f7f\u7528\u793a\u4f8b",id:"2\u4f7f\u7528\u793a\u4f8b",level:2},{value:"2.1.\u5b9e\u4f8b\u5316client\u5bf9\u8c61",id:"21\u5b9e\u4f8b\u5316client\u5bf9\u8c61",level:3},{value:"2.1.1.\u5b9e\u4f8b\u5316\u5355\u8282\u70b9client\u5bf9\u8c61",id:"211\u5b9e\u4f8b\u5316\u5355\u8282\u70b9client\u5bf9\u8c61",level:4},{value:"2.1.2.\u5b9e\u4f8b\u5316HA\u96c6\u7fa4\u76f4\u63a5\u8fde\u63a5client\u5bf9\u8c61",id:"212\u5b9e\u4f8b\u5316ha\u96c6\u7fa4\u76f4\u63a5\u8fde\u63a5client\u5bf9\u8c61",level:4},{value:"2.1.3.\u5b9e\u4f8b\u5316HA\u96c6\u7fa4\u95f4\u63a5\u8fde\u63a5client\u5bf9\u8c61",id:"213\u5b9e\u4f8b\u5316ha\u96c6\u7fa4\u95f4\u63a5\u8fde\u63a5client\u5bf9\u8c61",level:4},{value:"2.2.\u8c03\u7528cypher",id:"22\u8c03\u7528cypher",level:3},{value:"2.3.\u5411leader\u53d1\u9001cypher\u8bf7\u6c42",id:"23\u5411leader\u53d1\u9001cypher\u8bf7\u6c42",level:3},{value:"2.4.\u8c03\u7528GQL",id:"24\u8c03\u7528gql",level:3},{value:"2.5.\u5411leader\u53d1\u9001GQL\u8bf7\u6c42",id:"25\u5411leader\u53d1\u9001gql\u8bf7\u6c42",level:3},{value:"2.6.\u8c03\u7528\u5b58\u50a8\u8fc7\u7a0b",id:"26\u8c03\u7528\u5b58\u50a8\u8fc7\u7a0b",level:3},{value:"2.7.\u5411leader\u8c03\u7528\u5b58\u50a8\u8fc7\u7a0b",id:"27\u5411leader\u8c03\u7528\u5b58\u50a8\u8fc7\u7a0b",level:3},{value:"2.8.\u52a0\u8f7d\u5b58\u50a8\u8fc7\u7a0b",id:"28\u52a0\u8f7d\u5b58\u50a8\u8fc7\u7a0b",level:3},{value:"2.9.\u5217\u4e3e\u5b58\u50a8\u8fc7\u7a0b",id:"29\u5217\u4e3e\u5b58\u50a8\u8fc7\u7a0b",level:3},{value:"2.10.\u5220\u9664\u5b58\u50a8\u8fc7\u7a0b",id:"210\u5220\u9664\u5b58\u50a8\u8fc7\u7a0b",level:3},{value:"2.11.\u4ece\u5b57\u8282\u6d41\u4e2d\u5bfc\u5165schema",id:"211\u4ece\u5b57\u8282\u6d41\u4e2d\u5bfc\u5165schema",level:3},{value:"2.12.\u4ece\u5b57\u8282\u6d41\u4e2d\u5bfc\u5165\u70b9\u8fb9\u6570\u636e",id:"212\u4ece\u5b57\u8282\u6d41\u4e2d\u5bfc\u5165\u70b9\u8fb9\u6570\u636e",level:3},{value:"2.13.\u4ece\u6587\u4ef6\u4e2d\u5bfc\u5165schema",id:"213\u4ece\u6587\u4ef6\u4e2d\u5bfc\u5165schema",level:3},{value:"2.14.\u4ece\u6587\u4ef6\u4e2d\u5bfc\u5165\u70b9\u8fb9\u6570\u636e",id:"214\u4ece\u6587\u4ef6\u4e2d\u5bfc\u5165\u70b9\u8fb9\u6570\u636e",level:3}];function d(e){const n={blockquote:"blockquote",code:"code",h1:"h1",h2:"h2",h3:"h3",h4:"h4",header:"header",p:"p",pre:"pre",...(0,s.R)(),...e.components};return(0,t.jsxs)(t.Fragment,{children:[(0,t.jsx)(n.header,{children:(0,t.jsx)(n.h1,{id:"c\u5ba2\u6237\u7aef",children:"C++\u5ba2\u6237\u7aef"})}),"\n",(0,t.jsxs)(n.blockquote,{children:["\n",(0,t.jsx)(n.p,{children:"\u6b64\u6587\u6863\u4e3b\u8981\u662fTuGraph C++ SDK\u7684\u4f7f\u7528\u8bf4\u660e\u3002"}),"\n"]}),"\n",(0,t.jsx)(n.h2,{id:"1\u6982\u8ff0",children:"1.\u6982\u8ff0"}),"\n",(0,t.jsx)(n.p,{children:"C++ Client \u80fd\u591f\u4f7f\u7528 RPC \u8fde\u63a5lgraph_server\uff0c\u8fdb\u884c\u6570\u636e\u5bfc\u5165\u3001\u6267\u884c\u5b58\u50a8\u8fc7\u7a0b\u3001\u8c03\u7528Cypher\u7b49\u64cd\u4f5c\u3002"}),"\n",(0,t.jsx)(n.h2,{id:"2\u4f7f\u7528\u793a\u4f8b",children:"2.\u4f7f\u7528\u793a\u4f8b"}),"\n",(0,t.jsx)(n.h3,{id:"21\u5b9e\u4f8b\u5316client\u5bf9\u8c61",children:"2.1.\u5b9e\u4f8b\u5316client\u5bf9\u8c61"}),"\n",(0,t.jsx)(n.p,{children:"\u5f15\u5165\u4f9d\u8d56\u5e76\u5b9e\u4f8b\u5316"}),"\n",(0,t.jsx)(n.h4,{id:"211\u5b9e\u4f8b\u5316\u5355\u8282\u70b9client\u5bf9\u8c61",children:"2.1.1.\u5b9e\u4f8b\u5316\u5355\u8282\u70b9client\u5bf9\u8c61"}),"\n",(0,t.jsx)(n.p,{children:"\u5f53\u4ee5\u5355\u8282\u70b9\u6a21\u5f0f\u542f\u52a8server\u65f6\uff0cclient\u6309\u7167\u5982\u4e0b\u683c\u5f0f\u8fdb\u884c\u5b9e\u4f8b\u5316"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'RpcClient client("127.0.0.1:19099", "admin", "73@TuGraph");\n'})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:"RpcClient(const std::string& url, const std::string& user, const std::string& password);\n@param url: tugraph host looks like ip:port\n@param user: login user name\n@param password: login password\n"})}),"\n",(0,t.jsx)(n.h4,{id:"212\u5b9e\u4f8b\u5316ha\u96c6\u7fa4\u76f4\u63a5\u8fde\u63a5client\u5bf9\u8c61",children:"2.1.2.\u5b9e\u4f8b\u5316HA\u96c6\u7fa4\u76f4\u63a5\u8fde\u63a5client\u5bf9\u8c61"}),"\n",(0,t.jsx)(n.p,{children:"\u5f53\u670d\u52a1\u5668\u4e0a\u90e8\u7f72\u7684HA\u96c6\u7fa4\u53ef\u4ee5\u4f7f\u7528ha_conf\u4e2d\u914d\u7f6e\u7684\u7f51\u5740\u76f4\u63a5\u8fde\u63a5\u65f6\uff0cclient\u6309\u7167\u5982\u4e0b\u683c\u5f0f\u8fdb\u884c\u5b9e\u4f8b\u5316"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'RpcClient client("127.0.0.1:19099", "admin", "73@TuGraph");\n'})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:"RpcClient(const std::string& url, const std::string& user, const std::string& password);\n@param url: tugraph host looks like ip:port\n@param user: login user name \n@param password: login password\n"})}),"\n",(0,t.jsx)(n.p,{children:"\u7528\u6237\u53ea\u9700\u8981\u4f20\u5165HA\u96c6\u7fa4\u4e2d\u7684\u4efb\u610f\u4e00\u4e2a\u8282\u70b9\u7684url\u5373\u53ef\uff0cclient\u4f1a\u6839\u636eserver\u7aef\u8fd4\u56de\u7684\u67e5\u8be2\u4fe1\u606f\u81ea\u52a8\u7ef4\u62a4\u8fde\u63a5\u6c60\uff0c\u5728HA\u96c6\u7fa4\u6a2a\u5411\u6269\u5bb9\u65f6\n\u4e5f\u4e0d\u9700\u8981\u624b\u52a8\u91cd\u542fclient\u3002"}),"\n",(0,t.jsx)(n.h4,{id:"213\u5b9e\u4f8b\u5316ha\u96c6\u7fa4\u95f4\u63a5\u8fde\u63a5client\u5bf9\u8c61",children:"2.1.3.\u5b9e\u4f8b\u5316HA\u96c6\u7fa4\u95f4\u63a5\u8fde\u63a5client\u5bf9\u8c61"}),"\n",(0,t.jsx)(n.p,{children:"\u5f53\u670d\u52a1\u5668\u4e0a\u90e8\u7f72\u7684HA\u96c6\u7fa4\u4e0d\u80fd\u4f7f\u7528ha_conf\u4e2d\u914d\u7f6e\u7684\u7f51\u5740\u76f4\u63a5\u8fde\u63a5\u800c\u5fc5\u987b\u4f7f\u7528\u95f4\u63a5\u7f51\u5740\uff08\u5982\u963f\u91cc\u4e91\u516c\u7f51\u7f51\u5740\uff09\u8fde\u63a5\u65f6\uff0c\nclient\u6309\u7167\u5982\u4e0b\u683c\u5f0f\u8fdb\u884c\u5b9e\u4f8b\u5316\u3002"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-java",children:'std::vector<std::string> urls = {"189.33.97.23:9091", "189.33.97.24:9091", "189.33.97.25:9091"};\nTuGraphDbRpcClient client = new TuGraphDbRpcClient(urls, "admin", "73@TuGraph");\n'})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:"RpcClient(std::vector<std::string>& urls, std::string user, std::string password)\n@param urls: tugraph host list\n@param user: login user name\n@param password: login password\n"})}),"\n",(0,t.jsx)(n.p,{children:"\u56e0\u4e3a\u7528\u6237\u8fde\u63a5\u7684\u7f51\u5740\u548cserver\u542f\u52a8\u65f6\u914d\u7f6e\u7684\u4fe1\u606f\u4e0d\u540c\uff0c\u4e0d\u80fd\u901a\u8fc7\u5411\u96c6\u7fa4\u53d1\u8bf7\u6c42\u7684\u65b9\u5f0f\u81ea\u52a8\u66f4\u65b0client\u8fde\u63a5\u6c60\uff0c\u6240\u4ee5\u9700\u8981\u5728\u542f\u52a8\nclient\u65f6\u624b\u52a8\u4f20\u5165\u6240\u6709\u96c6\u7fa4\u4e2d\u8282\u70b9\u7684\u7f51\u5740\uff0c\u5e76\u5728\u96c6\u7fa4\u8282\u70b9\u53d8\u66f4\u65f6\u624b\u52a8\u91cd\u542fclient\u3002"}),"\n",(0,t.jsx)(n.h3,{id:"22\u8c03\u7528cypher",children:"2.2.\u8c03\u7528cypher"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:"    std::string str;\n    bool ret = client.CallCypher(str,\n        \"CALL db.createVertexLabel('actor', 'name', 'name', string, false, 'age', int8, true)\");\n\n"})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:'    bool CallCypher(std::string& result, const std::string& cypher,\n                    const std::string& graph = "default", bool json_format = true,\n                    double timeout = 0, const std::string& url = "");\n    @param [out] result      The result.\n    @param [in]  cypher      inquire statement.\n    @param [in]  graph       (Optional) the graph to query.\n    @param [in]  json_format (Optional) Returns the format\uff0c true is json\uff0cOtherwise, binary\n                             format.\n    @param [in]  timeout     (Optional) Maximum execution time, overruns will be interrupted.\n    @param [in]  url         (Optional) Node address of calling cypher.\n    @returns True if it succeeds, false if it fails.\n'})}),"\n",(0,t.jsx)(n.p,{children:"\u672c\u63a5\u53e3\u652f\u6301\u5728\u5355\u673a\u6a21\u5f0f\u548cHA\u6a21\u5f0f\u4e0b\u4f7f\u7528\u3002\u5176\u4e2d\uff0c\u5728HA\u6a21\u5f0f\u4e0b\u7684client\u4e2d\uff0c\u901a\u8fc7\u6307\u5b9aurl\u53c2\u6570\u53ef\u4ee5\u5b9a\u5411\u5411\u67d0\u4e2aserver\u53d1\u9001\u8bfb\u8bf7\u6c42\u3002"}),"\n",(0,t.jsx)(n.h3,{id:"23\u5411leader\u53d1\u9001cypher\u8bf7\u6c42",children:"2.3.\u5411leader\u53d1\u9001cypher\u8bf7\u6c42"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:"    std::string str;\n    bool ret = client.CallCypherToLeader(str,\n        \"CALL db.createVertexLabel('actor', 'name', 'name', string, false, 'age', int8, true)\");\n"})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:'    bool CallCypherToLeader(std::string& result, const std::string& cypher,\n                    const std::string& graph = "default", bool json_format = true,\n                    double timeout = 0);\n    @param [out] result      The result.\n    @param [in]  cypher      inquire statement.\n    @param [in]  graph       (Optional) the graph to query.\n    @param [in]  json_format (Optional) Returns the format\uff0c true is json\uff0cOtherwise, binary\n                             format.\n    @param [in]  timeout     (Optional) Maximum execution time, overruns will be interrupted.\n    @returns True if it succeeds, false if it fails.\n'})}),"\n",(0,t.jsx)(n.p,{children:"\u672c\u63a5\u53e3\u53ea\u652f\u6301\u5728HA\u6a21\u5f0f\u4e0b\u4f7f\u7528\uff0c\u5728HA\u6a21\u5f0f\u4e0b\u7684client\u4e2d\uff0c\u4e3a\u9632\u6b62\u5411\u672a\u540c\u6b65\u6570\u636e\u7684follower\u53d1\u9001\u8bf7\u6c42\uff0c\n\u7528\u6237\u53ef\u4ee5\u76f4\u63a5\u5411leader\u53d1\u9001\u8bf7\u6c42\uff0cleader\u7531\u96c6\u7fa4\u9009\u51fa\u3002"}),"\n",(0,t.jsx)(n.h3,{id:"24\u8c03\u7528gql",children:"2.4.\u8c03\u7528GQL"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:"    std::string str;\n    bool ret = client.CallGql(str,\n        \"CALL db.createVertexLabel('actor', 'name', 'name', string, false, 'age', int8, true)\");\n"})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:'    bool CallGql(std::string& result, const std::string& gql,\n                    const std::string& graph = "default", bool json_format = true,\n                    double timeout = 0, const std::string& url = "");\n    @param [out] result      The result.\n    @param [in]  gql         inquire statement.\n    @param [in]  graph       (Optional) the graph to query.\n    @param [in]  json_format (Optional) Returns the format\uff0c true is json\uff0cOtherwise, binary\n                             format.\n    @param [in]  timeout     (Optional) Maximum execution time, overruns will be interrupted.\n    @param [in]  url         (Optional) Node address of calling gql.\n    @returns True if it succeeds, false if it fails.\n'})}),"\n",(0,t.jsx)(n.p,{children:"\u672c\u63a5\u53e3\u652f\u6301\u5728\u5355\u673a\u6a21\u5f0f\u548cHA\u6a21\u5f0f\u4e0b\u4f7f\u7528\u3002\u5176\u4e2d\uff0c\u5728HA\u6a21\u5f0f\u4e0b\u7684client\u4e2d\uff0c\u901a\u8fc7\u6307\u5b9aurl\u53c2\u6570\u53ef\u4ee5\u5b9a\u5411\u5411\u67d0\u4e2aserver\u53d1\u9001\u8bfb\u8bf7\u6c42\u3002"}),"\n",(0,t.jsx)(n.h3,{id:"25\u5411leader\u53d1\u9001gql\u8bf7\u6c42",children:"2.5.\u5411leader\u53d1\u9001GQL\u8bf7\u6c42"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:"    std::string str;\n    bool ret = client.CallGqlToLeader(str,\n        \"CALL db.createVertexLabel('actor', 'name', 'name', string, false, 'age', int8, true)\");\n"})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:'    bool CallGqlToLeader(std::string& result, const std::string& gql,\n                 const std::string& graph = "default", bool json_format = true,\n                 double timeout = 0);\n    @param [out] result      The result.\n    @param [in]  gql         inquire statement.\n    @param [in]  graph       (Optional) the graph to query.\n    @param [in]  json_format (Optional) Returns the format\uff0c true is json\uff0cOtherwise, binary\n                             format.\n    @param [in]  timeout     (Optional) Maximum execution time, overruns will be interrupted.\n    @returns True if it succeeds, false if it fails.\n'})}),"\n",(0,t.jsx)(n.p,{children:"\u672c\u63a5\u53e3\u53ea\u652f\u6301\u5728HA\u6a21\u5f0f\u4e0b\u4f7f\u7528\uff0c\u5728HA\u6a21\u5f0f\u4e0b\u7684client\u4e2d\uff0c\u4e3a\u9632\u6b62\u5411\u672a\u540c\u6b65\u6570\u636e\u7684follower\u53d1\u9001\u8bf7\u6c42\uff0c\n\u7528\u6237\u53ef\u4ee5\u76f4\u63a5\u5411leader\u53d1\u9001\u8bf7\u6c42\uff0cleader\u7531\u96c6\u7fa4\u9009\u51fa\u3002"}),"\n",(0,t.jsx)(n.h3,{id:"26\u8c03\u7528\u5b58\u50a8\u8fc7\u7a0b",children:"2.6.\u8c03\u7528\u5b58\u50a8\u8fc7\u7a0b"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'    std::string str;\n    bool ret = client.CallProcedure(str, "CPP", "test_plugin1", "bcefg");\n'})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:'    bool CallProcedure(std::string& result, const std::string& procedure_type,\n                       const std::string& procedure_name, const std::string& param,\n                       double procedure_time_out = 0.0, bool in_process = false,\n                       const std::string& graph = "default", bool json_format = true,\n                       const std::string& url = "");\n    @param [out] result              The result.\n    @param [in]  procedure_type      the procedure type, currently supported CPP and PY.\n    @param [in]  procedure_name      procedure name.\n    @param [in]  param               the execution parameters.\n    @param [in]  procedure_time_out  (Optional) Maximum execution time, overruns will be\n                                     interrupted.\n    @param [in]  in_process          (Optional) support in future.\n    @param [in]  graph               (Optional) the graph to query.\n    @param [in]  json_format         (Optional) Returns the format\uff0c true is json\uff0cOtherwise,\n                                     binary format.\n    @param [in]  url                 (Optional) Node address of calling procedure.\n    @returns True if it succeeds, false if it fails.\n'})}),"\n",(0,t.jsx)(n.p,{children:"\u672c\u63a5\u53e3\u652f\u6301\u5728\u5355\u673a\u6a21\u5f0f\u548cHA\u6a21\u5f0f\u4e0b\u4f7f\u7528\uff0c\u9ed8\u8ba4\u4ee5json\u683c\u5f0f\u76f4\u63a5\u8fd4\u56de\u5b58\u50a8\u8fc7\u7a0b\u7684\u6267\u884c\u7ed3\u679c\uff0c\u6307\u5b9ajsonFormat\u4e3afalse\u53ef\u4ee5\u8fd4\u56de\u5b57\u7b26\u4e32\u683c\u5f0f\u7684\u6267\u884c\u7ed3\u679c\u3002\n\u5176\u4e2d\uff0c\u5728HA\u6a21\u5f0f\u4e0b\u7684client\u4e2d\uff0c\u901a\u8fc7\u6307\u5b9aurl\u53c2\u6570\u53ef\u4ee5\u5b9a\u5411\u5411\u67d0\u4e2aserver\u53d1\u9001\u8bfb\u8bf7\u6c42\u3002"}),"\n",(0,t.jsx)(n.h3,{id:"27\u5411leader\u8c03\u7528\u5b58\u50a8\u8fc7\u7a0b",children:"2.7.\u5411leader\u8c03\u7528\u5b58\u50a8\u8fc7\u7a0b"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'    std::string str;\n    bool ret = client.CallProcedureToLeader(str, "CPP", "test_plugin1", "bcefg");\n'})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:'    bool CallProcedureToLeader(std::string& result, const std::string& procedure_type,\n                       const std::string& procedure_name, const std::string& param,\n                       double procedure_time_out = 0.0, bool in_process = false,\n                       const std::string& graph = "default", bool json_format = true);\n    @param [out] result              The result.\n    @param [in]  procedure_type      the procedure type, currently supported CPP and PY.\n    @param [in]  procedure_name      procedure name.\n    @param [in]  param               the execution parameters.\n    @param [in]  procedure_time_out  (Optional) Maximum execution time, overruns will be\n                                     interrupted.\n    @param [in]  in_process          (Optional) support in future.\n    @param [in]  graph               (Optional) the graph to query.\n    @param [in]  json_format         (Optional) Returns the format\uff0c true is json\uff0cOtherwise,\n                                     binary format.\n    @returns True if it succeeds, false if it fails.\n'})}),"\n",(0,t.jsx)(n.p,{children:"\u672c\u63a5\u53e3\u652f\u6301\u5728HA\u6a21\u5f0f\u4e0b\u4f7f\u7528\uff0c\u9ed8\u8ba4\u4ee5json\u683c\u5f0f\u76f4\u63a5\u8fd4\u56de\u5b58\u50a8\u8fc7\u7a0b\u7684\u6267\u884c\u7ed3\u679c\uff0c\u6307\u5b9ajsonFormat\u4e3afalse\u53ef\u4ee5\u8fd4\u56de\u5b57\u7b26\u4e32\u683c\u5f0f\u7684\u6267\u884c\u7ed3\u679c\u3002"}),"\n",(0,t.jsx)(n.h3,{id:"28\u52a0\u8f7d\u5b58\u50a8\u8fc7\u7a0b",children:"2.8.\u52a0\u8f7d\u5b58\u50a8\u8fc7\u7a0b"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'    std::string str;\n    bool ret = client.LoadProcedure(str, code_sleep, "PY", "python_plugin1", "PY", "this is a test plugin", true)\n'})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:'    bool LoadProcedure(std::string& result, const std::string& source_file,\n                       const std::string& procedure_type, const std::string& procedure_name,\n                       const std::string& code_type, const std::string& procedure_description,\n                       bool read_only, const std::string& version = "v1",\n                       const std::string& graph = "default");\n    @param [out] result                  The result.\n    @param [in]  source_file             the source_file contain procedure code.\n    @param [in]  procedure_type          the procedure type, currently supported CPP and PY.\n    @param [in]  procedure_name          procedure name.\n    @param [in]  code_type               code type, currently supported PY, SO, CPP, ZIP.\n    @param [in]  procedure_description   procedure description.\n    @param [in]  read_only               procedure is read only or not.\n    @param [in]  version                 (Optional) the version of procedure.\n    @param [in]  graph                   (Optional) the graph to query.\n    @returns True if it succeeds, false if it fails.\n'})}),"\n",(0,t.jsx)(n.p,{children:"\u672c\u63a5\u53e3\u652f\u6301\u5728\u5355\u673a\u6a21\u5f0f\u548cHA\u6a21\u5f0f\u4e0b\u4f7f\u7528\u3002\u5176\u4e2d\uff0c\u7531\u4e8e\u52a0\u8f7d\u5b58\u50a8\u8fc7\u7a0b\u662f\u5199\u8bf7\u6c42\uff0cHA\u6a21\u5f0f\u4e0b\u7684client\u53ea\u80fd\u5411leader\u53d1\u9001\u52a0\u8f7d\u5b58\u50a8\u8fc7\u7a0b\u8bf7\u6c42\u3002"}),"\n",(0,t.jsx)(n.h3,{id:"29\u5217\u4e3e\u5b58\u50a8\u8fc7\u7a0b",children:"2.9.\u5217\u4e3e\u5b58\u50a8\u8fc7\u7a0b"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:"    std::string str;\n    bool ret = client.ListProcedures(str);\n"})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:'    bool ListProcedures(std::string& result, const std::string& procedure_type,\n                        const std::string& version = "any",\n                        const std::string& graph = "default", const std::string& url = "");\n    @param [out] result          The result.\n    @param [in]  procedure_type  (Optional) the procedure type, "" for all procedures,\n                                 CPP and PY for special type.\n    @param [in]  version         (Optional) the version of procedure.\n    @param [in]  graph           (Optional) the graph to query.\n    @param [in]  url             Node address of calling procedure.\n    @returns True if it succeeds, false if it fails.\n'})}),"\n",(0,t.jsx)(n.p,{children:"\u672c\u63a5\u53e3\u652f\u6301\u5728\u5355\u673a\u6a21\u5f0f\u548cHA\u6a21\u5f0f\u4e0b\u4f7f\u7528\u3002\u5176\u4e2d\uff0c\u5728HA\u6a21\u5f0f\u4e0b\u7684client\u4e2d\uff0c\u901a\u8fc7\u6307\u5b9aurl\u53c2\u6570\u53ef\u4ee5\u5b9a\u5411\u5411\u67d0\u4e2aserver\u53d1\u9001\u8bf7\u6c42\u3002"}),"\n",(0,t.jsx)(n.h3,{id:"210\u5220\u9664\u5b58\u50a8\u8fc7\u7a0b",children:"2.10.\u5220\u9664\u5b58\u50a8\u8fc7\u7a0b"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'    std::string str;\n    bool ret = client.DeleteProcedure(str, "CPP", "test_plugin1");\n'})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:'    bool DeleteProcedure(std::string& result, const std::string& procedure_type,\n                         const std::string& procedure_name, const std::string& graph = "default");\n    @param [out] result              The result.\n    @param [in]  procedure_type      the procedure type, currently supported CPP and PY.\n    @param [in]  procedure_name      procedure name.\n    @param [in]  graph               (Optional) the graph to query.\n    @returns True if it succeeds, false if it fails.\n'})}),"\n",(0,t.jsx)(n.p,{children:"\u672c\u63a5\u53e3\u652f\u6301\u5728\u5355\u673a\u6a21\u5f0f\u548cHA\u6a21\u5f0f\u4e0b\u4f7f\u7528\u3002\u5176\u4e2d\uff0c\u7531\u4e8e\u5220\u9664\u5b58\u50a8\u8fc7\u7a0b\u662f\u5199\u8bf7\u6c42\uff0cHA\u6a21\u5f0f\u4e0b\u7684client\u53ea\u80fd\u5411leader\u53d1\u9001\u5220\u9664\u5b58\u50a8\u8fc7\u7a0b\u8bf7\u6c42\u3002"}),"\n",(0,t.jsx)(n.h3,{id:"211\u4ece\u5b57\u8282\u6d41\u4e2d\u5bfc\u5165schema",children:"2.11.\u4ece\u5b57\u8282\u6d41\u4e2d\u5bfc\u5165schema"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'    std::string str;\n    bool ret = client.ImportSchemaFromContent(str, sImportContent["schema"]);\n'})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:'    bool ImportSchemaFromContent(std::string& result, const std::string& schema,\n                                 const std::string& graph = "default", bool json_format = true,\n                                 double timeout = 0);\n    @param [out] result      The result.\n    @param [in]  schema      the schema to be imported.\n    @param [in]  graph       (Optional) the graph to query.\n    @param [in]  json_format (Optional) Returns the format\uff0c true is json\uff0cOtherwise, binary\n                             format.\n    @param [in]  timeout     (Optional) Maximum execution time, overruns will be interrupted.\n    @returns True if it succeeds, false if it fails.\n'})}),"\n",(0,t.jsx)(n.p,{children:"\u672c\u63a5\u53e3\u652f\u6301\u5728\u5355\u673a\u6a21\u5f0f\u548cHA\u6a21\u5f0f\u4e0b\u4f7f\u7528\u3002\u5176\u4e2d\uff0c\u7531\u4e8e\u5bfc\u5165schema\u662f\u5199\u8bf7\u6c42\uff0cHA\u6a21\u5f0f\u4e0b\u7684client\u53ea\u80fd\u5411leader\u53d1\u9001\u5bfc\u5165schema\u8bf7\u6c42\u3002"}),"\n",(0,t.jsx)(n.h3,{id:"212\u4ece\u5b57\u8282\u6d41\u4e2d\u5bfc\u5165\u70b9\u8fb9\u6570\u636e",children:"2.12.\u4ece\u5b57\u8282\u6d41\u4e2d\u5bfc\u5165\u70b9\u8fb9\u6570\u636e"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'    std::string str;\n    ret = client.ImportDataFromContent(str, sImportContent["person_desc"], sImportContent["person"],",");\n'})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:'    bool ImportDataFromContent(std::string& result, const std::string& desc,\n                               const std::string& data, const std::string& delimiter,\n                               bool continue_on_error = false, int thread_nums = 8,\n                               const std::string& graph = "default", bool json_format = true,\n                               double timeout = 0);\n    @param [out] result              The result.\n    @param [in]  desc                data format description.\n    @param [in]  data                the data to be imported.\n    @param [in]  delimiter           data separator.\n    @param [in]  continue_on_error   (Optional) whether to continue when importing data fails.\n    @param [in]  thread_nums         (Optional) maximum number of threads.\n    @param [in]  graph               (Optional) the graph to query.\n    @param [in]  json_format         (Optional) Returns the format\uff0c true is json\uff0cOtherwise,\n                                     binary format.\n    @param [in]  timeout             (Optional) Maximum execution time, overruns will be\n                                     interrupted.\n    @returns True if it succeeds, false if it fails.\n'})}),"\n",(0,t.jsx)(n.p,{children:"\u672c\u63a5\u53e3\u652f\u6301\u5728\u5355\u673a\u6a21\u5f0f\u548cHA\u6a21\u5f0f\u4e0b\u4f7f\u7528\u3002\u5176\u4e2d\uff0c\u7531\u4e8e\u5bfc\u5165\u70b9\u8fb9\u6570\u636e\u662f\u5199\u8bf7\u6c42\uff0cHA\u6a21\u5f0f\u4e0b\u7684client\u53ea\u80fd\u5411leader\u53d1\u9001\u5bfc\u5165\u70b9\u8fb9\u6570\u636e\u8bf7\u6c42\u3002"}),"\n",(0,t.jsx)(n.h3,{id:"213\u4ece\u6587\u4ef6\u4e2d\u5bfc\u5165schema",children:"2.13.\u4ece\u6587\u4ef6\u4e2d\u5bfc\u5165schema"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'    std::string conf_file("./yago.conf");\n    std::string str;\n    ret = client.ImportSchemaFromFile(str, conf_file);\n'})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:'    bool ImportSchemaFromFile(std::string& result, const std::string& schema_file,\n                              const std::string& graph = "default", bool json_format = true,\n                              double timeout = 0);\n    @param [out] result      The result.\n    @param [in]  schema_file the schema_file contain schema.\n    @param [in]  graph       (Optional) the graph to query.\n    @param [in]  json_format (Optional) Returns the format\uff0c true is json\uff0cOtherwise, binary\n                             format.\n    @param [in]  timeout     (Optional) Maximum execution time, overruns will be interrupted.\n    @returns True if it succeeds, false if it fails.\n'})}),"\n",(0,t.jsx)(n.p,{children:"\u672c\u63a5\u53e3\u652f\u6301\u5728\u5355\u673a\u6a21\u5f0f\u548cHA\u6a21\u5f0f\u4e0b\u4f7f\u7528\u3002\u5176\u4e2d\uff0c\u7531\u4e8e\u5bfc\u5165schema\u662f\u5199\u8bf7\u6c42\uff0cHA\u6a21\u5f0f\u4e0b\u7684client\u53ea\u80fd\u5411leader\u53d1\u9001\u5bfc\u5165schema\u8bf7\u6c42\u3002"}),"\n",(0,t.jsx)(n.h3,{id:"214\u4ece\u6587\u4ef6\u4e2d\u5bfc\u5165\u70b9\u8fb9\u6570\u636e",children:"2.14.\u4ece\u6587\u4ef6\u4e2d\u5bfc\u5165\u70b9\u8fb9\u6570\u636e"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'    std::string conf_file("./yago.conf");\n    std::string str;\n    ret = client.ImportDataFromFile(str, conf_file, ",");\n'})}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{children:'    bool ImportDataFromFile(std::string& result, const std::string& conf_file,\n                            const std::string& delimiter, bool continue_on_error = false,\n                            int thread_nums = 8, int skip_packages = 0,\n                            const std::string& graph = "default", bool json_format = true,\n                            double timeout = 0);\n    @param [out] result              The result.\n    @param [in]  conf_file           data file contain format description and data.\n    @param [in]  delimiter           data separator.\n    @param [in]  continue_on_error   (Optional) whether to continue when importing data fails.\n    @param [in]  thread_nums         (Optional) maximum number of threads.\n    @param [in]  skip_packages       (Optional) skip packages number.\n    @param [in]  graph               (Optional) the graph to query.\n    @param [in]  json_format         (Optional) Returns the format\uff0c true is json\uff0cOtherwise,\n                                     binary format.\n    @param [in]  timeout             (Optional) Maximum execution time, overruns will be\n                                     interrupted.\n    @returns True if it succeeds, false if it fails.\n'})}),"\n",(0,t.jsx)(n.p,{children:"\u672c\u63a5\u53e3\u652f\u6301\u5728\u5355\u673a\u6a21\u5f0f\u548cHA\u6a21\u5f0f\u4e0b\u4f7f\u7528\u3002\u5176\u4e2d\uff0c\u7531\u4e8e\u5bfc\u5165\u70b9\u8fb9\u6570\u636e\u662f\u5199\u8bf7\u6c42\uff0cHA\u6a21\u5f0f\u4e0b\u7684client\u53ea\u80fd\u5411leader\u53d1\u9001\u5bfc\u5165\u70b9\u8fb9\u6570\u636e\u8bf7\u6c42\u3002"})]})}function p(e={}){const{wrapper:n}={...(0,s.R)(),...e.components};return n?(0,t.jsx)(n,{...e,children:(0,t.jsx)(d,{...e})}):d(e)}}}]);