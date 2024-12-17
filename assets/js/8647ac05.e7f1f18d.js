"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[91579],{12988:(e,o,r)=>{r.r(o),r.d(o,{assets:()=>d,contentTitle:()=>i,default:()=>h,frontMatter:()=>l,metadata:()=>s,toc:()=>a});var n=r(74848),t=r(28453);const l={},i="Log",s={id:"developer-manual/ecosystem-tools/log",title:"Log",description:"This document mainly introduces the logging function of TuGraph.",source:"@site/versions/version-3.6.0/en-US/source/5.developer-manual/5.ecosystem-tools/4.log.md",sourceDirName:"5.developer-manual/5.ecosystem-tools",slug:"/developer-manual/ecosystem-tools/log",permalink:"/tugraph-db/en/3.6.0/developer-manual/ecosystem-tools/log",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:4,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"TuGraph Explorer",permalink:"/tugraph-db/en/3.6.0/developer-manual/ecosystem-tools/tugraph-explorer"},next:{title:"Cypher API",permalink:"/tugraph-db/en/3.6.0/developer-manual/interface/cypher"}},d={},a=[{value:"1.Introduction",id:"1introduction",level:2},{value:"2.Server log",id:"2server-log",level:2},{value:"2.1.Server Log Categories",id:"21server-log-categories",level:3},{value:"2.2.Server Log Configuration Items",id:"22server-log-configuration-items",level:3},{value:"2.3.Example of Server Log Output Macro Usage",id:"23example-of-server-log-output-macro-usage",level:3},{value:"3.Audit log",id:"3audit-log",level:2}];function c(e){const o={blockquote:"blockquote",code:"code",h1:"h1",h2:"h2",h3:"h3",header:"header",p:"p",pre:"pre",...(0,t.R)(),...e.components};return(0,n.jsxs)(n.Fragment,{children:[(0,n.jsx)(o.header,{children:(0,n.jsx)(o.h1,{id:"log",children:"Log"})}),"\n",(0,n.jsxs)(o.blockquote,{children:["\n",(0,n.jsx)(o.p,{children:"This document mainly introduces the logging function of TuGraph."}),"\n"]}),"\n",(0,n.jsx)(o.h2,{id:"1introduction",children:"1.Introduction"}),"\n",(0,n.jsx)(o.p,{children:"TuGraph keeps two types of logs: server logs and audit logs. Server logs record human-readable server status information, while audit logs maintain encrypted information for every operation performed on the server."}),"\n",(0,n.jsx)(o.h2,{id:"2server-log",children:"2.Server log"}),"\n",(0,n.jsx)(o.h3,{id:"21server-log-categories",children:"2.1.Server Log Categories"}),"\n",(0,n.jsxs)(o.p,{children:["The server logs are currently divided into three categories: ",(0,n.jsx)(o.code,{children:"general log"}),", ",(0,n.jsx)(o.code,{children:"debug log"}),", and ",(0,n.jsx)(o.code,{children:"query log"}),"."]}),"\n",(0,n.jsxs)(o.p,{children:["The ",(0,n.jsx)(o.code,{children:"general log"})," is for regular users and includes important information related to the database startup and shutdown, such as configuration information during database startup, the port listened for services, and whether the database has successfully started or shut down. It provides a clear indication of whether the database is running."]}),"\n",(0,n.jsxs)(o.p,{children:["The ",(0,n.jsx)(o.code,{children:"debug log"})," is for database developers and includes debug-related information generated during runtime, such as network requests received by the database and the execution process of queries. It helps developers during development and developers can control the log level through the ",(0,n.jsx)(o.code,{children:"verbose"})," configuration item."]}),"\n",(0,n.jsxs)(o.p,{children:["(to be implemented)The ",(0,n.jsx)(o.code,{children:"query log"})," is for business developers and includes information about each query executed by database and the performance-related information of the database when executing the query, such as query execution time. It can filter queries whose execution time is lower than the specified threshold time through configuration setting, making it easier for business developers to analyze performance."]}),"\n",(0,n.jsx)(o.h3,{id:"22server-log-configuration-items",children:"2.2.Server Log Configuration Items"}),"\n",(0,n.jsxs)(o.p,{children:["The output directory of server logs can be specified through the ",(0,n.jsx)(o.code,{children:"log_dir"})," configuration item. The level of detail in the debug log can be specified through the ",(0,n.jsx)(o.code,{children:"verbose"})," configuration item."]}),"\n",(0,n.jsxs)(o.p,{children:["The ",(0,n.jsx)(o.code,{children:"log_dir"})," configuration item is empty by default. If ",(0,n.jsx)(o.code,{children:"log_dir"})," is empty, then all logs will be write to the console. If ",(0,n.jsx)(o.code,{children:"log_dir"})," is manually specified, the following log folder structure will be generated in the corresponding path:"]}),"\n",(0,n.jsx)(o.pre,{children:(0,n.jsx)(o.code,{children:".\n\u251c\u2500\u2500 debug.log\n\u251c\u2500\u2500 general.log\n\u251c\u2500\u2500 query.log(TODO)\n\u2514\u2500\u2500 history_logs\n    \u251c\u2500\u2500 debug_logs\n    \u251c\u2500\u2500 general_logs\n    \u2514\u2500\u2500 query_logs(TODO)\n"})}),"\n",(0,n.jsxs)(o.p,{children:["The ",(0,n.jsx)(o.code,{children:"general log"})," is wrote into ",(0,n.jsx)(o.code,{children:"general.log"})," file. The ",(0,n.jsx)(o.code,{children:"debug log"})," is wrote into ",(0,n.jsx)(o.code,{children:"debug.log"})," file. The ",(0,n.jsx)(o.code,{children:"query log"})," is wrote into ",(0,n.jsx)(o.code,{children:"query.log"})," file."]}),"\n",(0,n.jsxs)(o.p,{children:["The maximum size of a single log file is 64mb, and when it reaches the maximum file size, it will be rotated into the ",(0,n.jsx)(o.code,{children:"history_logs"})," folder. ",(0,n.jsx)(o.code,{children:"general.log"})," is rotated into the ",(0,n.jsx)(o.code,{children:"general_logs"})," folder, and ",(0,n.jsx)(o.code,{children:"debug.log"})," is rotated into the ",(0,n.jsx)(o.code,{children:"debug_logs"})," folder."]}),"\n",(0,n.jsxs)(o.p,{children:["The ",(0,n.jsx)(o.code,{children:"verbose"})," configuration item controls the level of detail of log records in ",(0,n.jsx)(o.code,{children:"debug log"}),", and is divided into three levels of ",(0,n.jsx)(o.code,{children:"0, 1, 2"}),". Ther verbosity of log record grows as the number grows. The default level is ",(0,n.jsx)(o.code,{children:"2"}),", which records the most detailed log records. The server will print all log records in ",(0,n.jsx)(o.code,{children:"DEBUG"})," level and above, and include the ",(0,n.jsx)(o.code,{children:"[filename:function:line_number]"})," of the log records being printed, which is convenient for debugging. When the level is set to ",(0,n.jsx)(o.code,{children:"1"}),", the server will only print major events log records in ",(0,n.jsx)(o.code,{children:"INFO"})," level and above. When the level is set to ",(0,n.jsx)(o.code,{children:"0"}),", the server will only print error log records in ",(0,n.jsx)(o.code,{children:"ERROR"})," level and above."]}),"\n",(0,n.jsx)(o.h3,{id:"23example-of-server-log-output-macro-usage",children:"2.3.Example of Server Log Output Macro Usage"}),"\n",(0,n.jsx)(o.p,{children:"If the developer wants to add logs to the code during development, they can refer to the following example:"}),"\n",(0,n.jsx)(o.pre,{children:(0,n.jsx)(o.code,{children:'#include "tools/lgraph_log.h" // add log dependency.\n\n\nvoid LogExample() {\n    // The log module has been initialized during the database startup, and developers can directly call the macro.\n    // The log level is divided into six levels: TRACE, DEBUG, INFO, WARNING, ERROR, and FATAL.\n\n    GENERAL_LOG(INFO) << "This is a info level general log message."; // general log\u7684\u8f93\u51fa\u5b8f\n\n    DEBUG_LOG(ERROR) << "This is a error level debug log message."; // debug log\u7684\u8f93\u51fa\u5b8f\n}\n'})}),"\n",(0,n.jsx)(o.p,{children:"You can also refer to the log macro usage in test/test_lgraph_log.cpp."}),"\n",(0,n.jsx)(o.h2,{id:"3audit-log",children:"3.Audit log"}),"\n",(0,n.jsx)(o.p,{children:"Audit logs record each request and response, as well as the user who sent the request and when the request received. Audit logging can only be turned on or off. The results can be queried using the TuGraph visualization tool and the REST API."})]})}function h(e={}){const{wrapper:o}={...(0,t.R)(),...e.components};return o?(0,n.jsx)(o,{...e,children:(0,n.jsx)(c,{...e})}):c(e)}},28453:(e,o,r)=>{r.d(o,{R:()=>i,x:()=>s});var n=r(96540);const t={},l=n.createContext(t);function i(e){const o=n.useContext(l);return n.useMemo((function(){return"function"==typeof e?e(o):{...o,...e}}),[o,e])}function s(e){let o;return o=e.disableParentContext?"function"==typeof e.components?e.components(t):e.components||t:i(e.components),n.createElement(l.Provider,{value:o},e.children)}}}]);