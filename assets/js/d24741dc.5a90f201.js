"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[62032],{25373:(e,n,r)=>{r.r(n),r.d(n,{assets:()=>l,contentTitle:()=>o,default:()=>u,frontMatter:()=>i,metadata:()=>a,toc:()=>c});var t=r(74848),s=r(28453);const i={},o="RPC API",a={id:"client-tools/rpc-api",title:"RPC API",description:"This document mainly introduces the details of calling TuGraph's RPC API.",source:"@site/versions/version-4.3.0/en-US/source/7.client-tools/8.rpc-api.md",sourceDirName:"7.client-tools",slug:"/client-tools/rpc-api",permalink:"/tugraph-db/en/4.3.0/client-tools/rpc-api",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:8,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"TuGraph RESTful API",permalink:"/tugraph-db/en/4.3.0/client-tools/restful-api"},next:{title:"TuGraph RESTful API Legacy",permalink:"/tugraph-db/en/4.3.0/client-tools/restful-api-legacy"}},l={},c=[{value:"1.Introduction",id:"1introduction",level:2},{value:"2.Request",id:"2request",level:2},{value:"2.1.Establishing a Connection",id:"21establishing-a-connection",level:3},{value:"2.2.Request Types",id:"22request-types",level:3},{value:"3.Login",id:"3login",level:2},{value:"4.Query",id:"4query",level:2},{value:"5.Stored Procedures",id:"5stored-procedures",level:2},{value:"5.1.Load Stored Procedures",id:"51load-stored-procedures",level:3},{value:"5.2.Invoke Stored Procedures",id:"52invoke-stored-procedures",level:3},{value:"5.3.Delete Stored Procedures",id:"53delete-stored-procedures",level:3},{value:"5.4.List Stored Procedures",id:"54list-stored-procedures",level:3}];function d(e){const n={blockquote:"blockquote",code:"code",h1:"h1",h2:"h2",h3:"h3",header:"header",li:"li",p:"p",pre:"pre",strong:"strong",table:"table",tbody:"tbody",td:"td",th:"th",thead:"thead",tr:"tr",ul:"ul",...(0,s.R)(),...e.components};return(0,t.jsxs)(t.Fragment,{children:[(0,t.jsx)(n.header,{children:(0,t.jsx)(n.h1,{id:"rpc-api",children:"RPC API"})}),"\n",(0,t.jsxs)(n.blockquote,{children:["\n",(0,t.jsx)(n.p,{children:"This document mainly introduces the details of calling TuGraph's RPC API."}),"\n"]}),"\n",(0,t.jsx)(n.h2,{id:"1introduction",children:"1.Introduction"}),"\n",(0,t.jsx)(n.p,{children:"TuGraph provides rich RPC APIs for developers to remotely call the services provided by TuGraph through RPC requests."}),"\n",(0,t.jsx)(n.p,{children:"RPC (Remote Procedure Call) is a protocol for requesting services from remote computer programs through a network without the need to understand the underlying network technology.\nCompared with REST, RPC is method-oriented and mainly used for calling function methods. It is suitable for more complex communication scenarios and has higher performance.\nbrpc is an industrial-grade RPC framework written in C++. Based on brpc, TuGraph provides rich RPC APIs. This document describes how to use TuGraph's RPC API."}),"\n",(0,t.jsx)(n.h2,{id:"2request",children:"2.Request"}),"\n",(0,t.jsx)(n.h3,{id:"21establishing-a-connection",children:"2.1.Establishing a Connection"}),"\n",(0,t.jsx)(n.p,{children:"When developers send RPC requests to TuGraph services, they must first establish a connection. Taking C++ as an example, developers create a channel with the specified URL and create a specified service stub (LGraphRPCService_Stub) from the channel. Subsequently, they can send requests to the remote server through the stub like calling local methods."}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'    std::shared_ptr<lgraph_rpc::m_channel_options> options = std::make_shared<lgraph_rpc::m_channel_options>();\n    options->protocol = "baidu_std";\n    options->connection_type = "";\n    options->timeout_ms = 60 * 60 * 1000 /*milliseconds*/;\n    options->max_retry = 3;\n    std::string load_balancer = "";\n    std::shared_ptr<lgraph_rpc::m_channel> channel = std::make_shared<lgraph_rpc::m_channel>();\n    if (channel->Init(url.c_str(), load_balancer, options.get()) != 0)\n        throw RpcException("Fail to initialize channel");\n    LGraphRPCService_Stub stub(channel.get());\n'})}),"\n",(0,t.jsx)(n.h3,{id:"22request-types",children:"2.2.Request Types"}),"\n",(0,t.jsx)(n.p,{children:"TuGraph supports 10 types of RPC requests, and each request's functionality is shown in the following table:"}),"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",(0,t.jsxs)(n.table,{children:[(0,t.jsx)(n.thead,{children:(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.th,{children:"Request"}),(0,t.jsx)(n.th,{children:"Functionality"})]})}),(0,t.jsxs)(n.tbody,{children:[(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"GraphApiRequest"}),(0,t.jsx)(n.td,{children:"Vertex-Edge Index"})]}),(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"CypherRequest"}),(0,t.jsx)(n.td,{children:"cypher"})]}),(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"PluginRequest"}),(0,t.jsx)(n.td,{children:"Stored Procedures"})]}),(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"HARequest"}),(0,t.jsx)(n.td,{children:"High Availability"})]}),(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"ImportRequest"}),(0,t.jsx)(n.td,{children:"Data Import"})]}),(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"GraphRequest"}),(0,t.jsx)(n.td,{children:"Subgraph Operations"})]}),(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"AclRequest"}),(0,t.jsx)(n.td,{children:"Access Control"})]}),(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"ConfigRequest"}),(0,t.jsx)(n.td,{children:"Configuration"})]}),(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"RestoreRequest"}),(0,t.jsx)(n.td,{children:"Backup"})]}),(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"SchemaRequest"}),(0,t.jsx)(n.td,{children:"Schema Management"})]})]})]}),"\n",(0,t.jsx)(n.p,{children:"When a user sends a request, the following parameters need to be passed in:"}),"\n",(0,t.jsxs)(n.ul,{children:["\n",(0,t.jsxs)(n.li,{children:["client_version: an optional parameter, in HA mode, it can prevent outdated responses by comparing ",(0,t.jsx)(n.code,{children:"client_version"})," and ",(0,t.jsx)(n.code,{children:"server_version"})]}),"\n",(0,t.jsx)(n.li,{children:"token: a necessary parameter, the client obtains the token after logging in, and the token is passed in with each request to verify the user's identity"}),"\n",(0,t.jsx)(n.li,{children:"is_write_op: an optional parameter, indicating whether the request is a write request"}),"\n",(0,t.jsx)(n.li,{children:"user: an optional parameter, set user when synchronizing requests between master and slave in HA mode, and no token verification is required After the service processes the RPC request, it sends back a response."}),"\n"]}),"\n",(0,t.jsx)(n.p,{children:"In addition to containing separate response information for each request, the response message also includes the following parameters:"}),"\n",(0,t.jsxs)(n.ul,{children:["\n",(0,t.jsx)(n.li,{children:"error_code: a necessary parameter, indicating the processing status of the request"}),"\n",(0,t.jsx)(n.li,{children:"redirect: an optional parameter, when processing fails to send a write request to a follower in HA mode, set redirect as the request forwarding address, that is, the leader address"}),"\n",(0,t.jsx)(n.li,{children:"error: an optional parameter, indicating the error information of the request"}),"\n",(0,t.jsxs)(n.li,{children:["server_version: an optional parameter, set ",(0,t.jsx)(n.code,{children:"server_version"})," in the HA mode request response to avoid reverse time travel when client reads data"]}),"\n"]}),"\n",(0,t.jsxs)(n.p,{children:["\u26a0\ufe0f  ",(0,t.jsx)(n.strong,{children:"Except for CypherRequest, PluginRequest, HARequest and AclRequest, all other RPC interfaces will be gradually deprecated, and their functions will be unified into the CypherRequest interface."})]}),"\n",(0,t.jsx)(n.h2,{id:"3login",children:"3.Login"}),"\n",(0,t.jsx)(n.p,{children:"The login request message contains the following parameters:"}),"\n",(0,t.jsxs)(n.ul,{children:["\n",(0,t.jsx)(n.li,{children:"user: a necessary parameter, the username"}),"\n",(0,t.jsx)(n.li,{children:"pass: a necessary parameter, the password"}),"\n"]}),"\n",(0,t.jsx)(n.p,{children:"Taking C++ as an example, the user sends a login request using the constructed service stub:"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:"    auto* req = request.mutable_acl_request();\n    auto* auth = req->mutable_auth_request()->mutable_login();\n    auth->set_user(user);\n    auth->set_password(pass);\n    // send data\n    cntl->Reset();\n    cntl->request_attachment().append(FLAGS_attachment);\n    req->set_client_version(server_version);\n    req->set_token(token);\n    LGraphRPCService_Stub stub(channel.get());\n    LGraphResponse res;\n    stub.HandleRequest(cntl.get(), req, &resp, nullptr);\n    if (cntl->Failed()) throw RpcConnectionException(cntl->ErrorText());\n    server_version = std::max(server_version, res.server_version());\n    if (res.error_code() != LGraphResponse::SUCCESS) throw RpcStatusException(res.error());\n    token = res.acl_response().auth_response().token();\n"})}),"\n",(0,t.jsx)(n.p,{children:"The login response message contains the following parameters:"}),"\n",(0,t.jsxs)(n.ul,{children:["\n",(0,t.jsx)(n.li,{children:'token: a necessary parameter. After successful login, a signed token, namely Json Web Token, will be received. The client stores the token and uses it for each subsequent request. If the login fails, an "Authentication failed" error will be received.'}),"\n"]}),"\n",(0,t.jsx)(n.h2,{id:"4query",children:"4.Query"}),"\n",(0,t.jsx)(n.p,{children:"Users can interact with TuGraph through Cypher queries. The Cypher request message contains the following parameters:"}),"\n",(0,t.jsxs)(n.ul,{children:["\n",(0,t.jsx)(n.li,{children:"query: a necessary parameter, the Cypher query statement"}),"\n",(0,t.jsx)(n.li,{children:"param_names: an optional parameter, the parameter name"}),"\n",(0,t.jsx)(n.li,{children:"param_values: an optional parameter, the parameter value"}),"\n",(0,t.jsx)(n.li,{children:"result_in_json_format: a necessary parameter, whether to return the query results in JSON format"}),"\n",(0,t.jsx)(n.li,{children:"graph: an optional parameter, the subgraph name for executing the Cypher statement"}),"\n",(0,t.jsx)(n.li,{children:"timeout: an optional parameter, the timeout for executing the Cypher statement"}),"\n"]}),"\n",(0,t.jsx)(n.p,{children:"Taking C++ as an example, the user sends a Cypher request as follows:"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:"    LGraphResponse res;\n    cntl->Reset();\n    cntl->request_attachment().append(FLAGS_attachment);\n    LGraphRequest req;\n    req.set_client_version(server_version);\n    req.set_token(token);\n    lgraph::CypherRequest* cypher_req = req.mutable_cypher_request();\n    cypher_req->set_graph(graph);\n    cypher_req->set_query(query);\n    cypher_req->set_timeout(timeout);\n    cypher_req->set_result_in_json_format(true);\n    LGraphRPCService_Stub stub(channel.get());\n    stub.HandleRequest(cntl.get(), &req, &res, nullptr);\n    if (cntl->Failed()) throw RpcConnectionException(cntl->ErrorText());\n    if (res.error_code() != LGraphResponse::SUCCESS) throw RpcStatusException(res.error());\n    server_version = std::max(server_version, res.server_version());\n    CypherResponse cypher_res = res.cypher_response();\n"})}),"\n",(0,t.jsx)(n.p,{children:"The Cypher request response contains one of the following two parameters:"}),"\n",(0,t.jsxs)(n.ul,{children:["\n",(0,t.jsx)(n.li,{children:"json_result: the Cypher query result in JSON format"}),"\n",(0,t.jsx)(n.li,{children:"binary_result: the Cypher query result in the CypherResult format"}),"\n"]}),"\n",(0,t.jsx)(n.h2,{id:"5stored-procedures",children:"5.Stored Procedures"}),"\n",(0,t.jsx)(n.p,{children:"To meet users' more complex query/update logic, TuGraph supports stored procedures written in C and Python. Users can use RPC requests to perform CRUD operations on stored procedures."}),"\n",(0,t.jsx)(n.h3,{id:"51load-stored-procedures",children:"5.1.Load Stored Procedures"}),"\n",(0,t.jsx)(n.p,{children:"The request for loading stored procedures contains the following parameters:"}),"\n",(0,t.jsxs)(n.ul,{children:["\n",(0,t.jsx)(n.li,{children:"name: a necessary parameter, the stored procedure name"}),"\n",(0,t.jsx)(n.li,{children:"read_only: a necessary parameter, whether it is read-only"}),"\n",(0,t.jsx)(n.li,{children:"code: a necessary parameter, the ByteString generated by reading the stored procedure file"}),"\n",(0,t.jsx)(n.li,{children:"desc: an optional parameter, the stored procedure description"}),"\n",(0,t.jsx)(n.li,{children:"code_type: an optional parameter, the stored procedure code type, which can be PY, SO, CPP, or ZIP"}),"\n"]}),"\n",(0,t.jsx)(n.p,{children:"Taking C++ as an example, the user loads the stored procedure as follows:"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'    std::string content;\n    if (!FieldSpecSerializer::FileReader(source_file, content)) {\n        std::swap(content, result);\n        return false;\n    }\n    LGraphRequest req;\n    req.set_is_write_op(true);\n    lgraph::PluginRequest* pluginRequest = req.mutable_plugin_request();\n    pluginRequest->set_graph(graph);\n    pluginRequest->set_type(procedure_type == "CPP" ? lgraph::PluginRequest::CPP\n                                                    : lgraph::PluginRequest::PYTHON);\n    pluginRequest->set_version(version);\n    lgraph::LoadPluginRequest* loadPluginRequest = pluginRequest->mutable_load_plugin_request();\n    loadPluginRequest->set_code_type([](const std::string& type) {\n        std::unordered_map<std::string, lgraph::LoadPluginRequest_CodeType> um{\n            {"SO", lgraph::LoadPluginRequest::SO},\n            {"PY", lgraph::LoadPluginRequest::PY},\n            {"ZIP", lgraph::LoadPluginRequest::ZIP},\n            {"CPP", lgraph::LoadPluginRequest::CPP}};\n        return um[type];\n    }(code_type));\n    loadPluginRequest->set_name(procedure_name);\n    loadPluginRequest->set_desc(procedure_description);\n    loadPluginRequest->set_read_only(read_only);\n    loadPluginRequest->set_code(content);\n    cntl->Reset();\n    cntl->request_attachment().append(FLAGS_attachment);\n    req.set_client_version(server_version);\n    req.set_token(token);\n    LGraphRPCService_Stub stub(channel.get());\n    LGraphResponse res;\n    stub.HandleRequest(cntl.get(), &req, &res, nullptr);\n    if (cntl->Failed()) throw RpcConnectionException(cntl->ErrorText());\n    server_version = std::max(server_version, res.server_version());\n    if (res.error_code() != LGraphResponse::SUCCESS) throw RpcStatusException(res.error());\n'})}),"\n",(0,t.jsx)(n.p,{children:"The response for loading the stored procedure does not contain parameters, and if the loading fails, a BadInput exception will be thrown."}),"\n",(0,t.jsx)(n.h3,{id:"52invoke-stored-procedures",children:"5.2.Invoke Stored Procedures"}),"\n",(0,t.jsx)(n.p,{children:"The request for invoking stored procedures contains the following parameters:"}),"\n",(0,t.jsxs)(n.ul,{children:["\n",(0,t.jsx)(n.li,{children:"name: a necessary parameter, the stored procedure name"}),"\n",(0,t.jsx)(n.li,{children:"param: a necessary parameter, the stored procedure parameters"}),"\n",(0,t.jsx)(n.li,{children:"result_in_json_format: an optional parameter, whether to return the invocation result in JSON format"}),"\n",(0,t.jsx)(n.li,{children:"in_process: an optional parameter, to be supported in the future"}),"\n",(0,t.jsx)(n.li,{children:"timeout: an optional parameter, the timeout for invoking the stored procedure"}),"\n"]}),"\n",(0,t.jsx)(n.p,{children:"Taking C++ as an example, the user invokes the stored procedure as follows:"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'    LGraphRequest req;\n    lgraph::PluginRequest* pluginRequest = req.mutable_plugin_request();\n    pluginRequest->set_graph(graph);\n    pluginRequest->set_type(procedure_type == "CPP" ? lgraph::PluginRequest::CPP\n                                                    : lgraph::PluginRequest::PYTHON);\n    lgraph::CallPluginRequest *cpRequest = pluginRequest->mutable_call_plugin_request();\n    cpRequest->set_name(procedure_name);\n    cpRequest->set_in_process(in_process);\n    cpRequest->set_param(param);\n    cpRequest->set_timeout(procedure_time_out);\n    cpRequest->set_result_in_json_format(json_format);\n    LGraphResponse res;\n    cntl->Reset();\n    cntl->request_attachment().append(FLAGS_attachment);\n    req.set_client_version(server_version);\n    req.set_token(token);\n    LGraphRPCService_Stub stub(channel.get());\n    stub.HandleRequest(cntl.get(), &req, &res, nullptr);\n    if (cntl->Failed()) throw RpcConnectionException(cntl->ErrorText());\n    server_version = std::max(server_version, res.server_version());\n    if (res.error_code() != LGraphResponse::SUCCESS) throw RpcStatusException(res.error());\n    if (json_format) {\n        result = res.mutable_plugin_response()->mutable_call_plugin_response()->json_result();\n    } else {\n        result = res.mutable_plugin_response()->mutable_call_plugin_response()->reply();\n    }\n'})}),"\n",(0,t.jsx)(n.p,{children:"The response for invoking the stored procedure contains one of the following two parameters:"}),"\n",(0,t.jsxs)(n.ul,{children:["\n",(0,t.jsx)(n.li,{children:"reply: the stored procedure invocation result in the ByteString format"}),"\n",(0,t.jsx)(n.li,{children:"json_result: the stored procedure invocation result in JSON format"}),"\n"]}),"\n",(0,t.jsx)(n.h3,{id:"53delete-stored-procedures",children:"5.3.Delete Stored Procedures"}),"\n",(0,t.jsx)(n.p,{children:"The request for deleting stored procedures contains the following parameters:"}),"\n",(0,t.jsxs)(n.ul,{children:["\n",(0,t.jsx)(n.li,{children:"name: a necessary parameter, the stored procedure name"}),"\n"]}),"\n",(0,t.jsx)(n.p,{children:"Taking C++ as an example, the user deletes the stored procedure as follows:"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'    LGraphRequest req;\n    req.set_is_write_op(true);\n    lgraph::PluginRequest* pluginRequest = req.mutable_plugin_request();\n    pluginRequest->set_graph(graph);\n    pluginRequest->set_type(procedure_type == "CPP" ? lgraph::PluginRequest::CPP\n                                                    : lgraph::PluginRequest::PYTHON);\n    lgraph::DelPluginRequest* dpRequest = pluginRequest->mutable_del_plugin_request();\n    dpRequest->set_name(procedure_name);\n    cntl->Reset();\n    cntl->request_attachment().append(FLAGS_attachment);\n    req.set_client_version(server_version);\n    req.set_token(token);\n    LGraphRPCService_Stub stub(channel.get());\n    LGraphResponse res;\n    stub.HandleRequest(cntl.get(), &req, &res, nullptr);\n    if (cntl->Failed()) throw RpcConnectionException(cntl->ErrorText());\n    server_version = std::max(server_version, res.server_version());\n    if (res.error_code() != LGraphResponse::SUCCESS) throw RpcStatusException(res.error());\n'})}),"\n",(0,t.jsx)(n.p,{children:"The response for deleting the stored procedure does not contain parameters, and if the deletion fails, a BadInput exception will be thrown."}),"\n",(0,t.jsx)(n.h3,{id:"54list-stored-procedures",children:"5.4.List Stored Procedures"}),"\n",(0,t.jsx)(n.p,{children:"The request for listing stored procedures does not require parameters. Taking C++ as an example, the user lists the stored procedures as follows:"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-C++",children:'    LGraphRequest req;\n    req.set_is_write_op(false);\n    lgraph::PluginRequest* pluginRequest = req.mutable_plugin_request();\n    pluginRequest->set_graph(graph);\n    pluginRequest->set_type(procedure_type == "CPP" ? lgraph::PluginRequest::CPP\n                                                    : lgraph::PluginRequest::PYTHON);\n    pluginRequest->mutable_list_plugin_request();\n    cntl->Reset();\n    cntl->request_attachment().append(FLAGS_attachment);\n    req.set_client_version(server_version);\n    req.set_token(token);\n    LGraphRPCService_Stub stub(channel.get());\n    LGraphResponse res;\n    stub.HandleRequest(cntl.get(), &req, &res, nullptr);\n    if (cntl->Failed()) throw RpcConnectionException(cntl->ErrorText());\n    server_version = std::max(server_version, res.server_version());\n    if (res.error_code() != LGraphResponse::SUCCESS) throw RpcStatusException(res.error());\n    result = res.mutable_plugin_response()->mutable_list_plugin_response()->reply();\n'})}),"\n",(0,t.jsx)(n.p,{children:"The response for listing the stored procedures contains the following parameter:"}),"\n",(0,t.jsxs)(n.ul,{children:["\n",(0,t.jsx)(n.li,{children:"reply: the procedure list in JSON format"}),"\n"]})]})}function u(e={}){const{wrapper:n}={...(0,s.R)(),...e.components};return n?(0,t.jsx)(n,{...e,children:(0,t.jsx)(d,{...e})}):d(e)}},28453:(e,n,r)=>{r.d(n,{R:()=>o,x:()=>a});var t=r(96540);const s={},i=t.createContext(s);function o(e){const n=t.useContext(i);return t.useMemo((function(){return"function"==typeof e?e(n):{...n,...e}}),[n,e])}function a(e){let n;return n=e.disableParentContext?"function"==typeof e.components?e.components(s):e.components||s:o(e.components),t.createElement(i.Provider,{value:n},e.children)}}}]);