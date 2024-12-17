"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[5593],{58057:(n,e,r)=>{r.r(e),r.d(e,{assets:()=>h,contentTitle:()=>t,default:()=>x,frontMatter:()=>l,metadata:()=>i,toc:()=>c});var s=r(74848),d=r(28453);const l={},t="\u6570\u636e\u5e93\u8fd0\u884c",i={id:"installation&running/tugraph-running",title:"\u6570\u636e\u5e93\u8fd0\u884c",description:"\u672c\u6587\u6863\u4e3b\u8981\u63cf\u8ff0 TuGraph \u670d\u52a1\u7684\u8fd0\u884c\u6a21\u5f0f\u3001\u542f\u52a8\u3001\u505c\u6b62\u548c\u91cd\u542f\u7684\u64cd\u4f5c,\u4ee5\u53ca TuGraph \u7684\u670d\u52a1\u914d\u7f6e\u53c2\u6570\u3001\u914d\u7f6e\u6587\u4ef6\u683c\u5f0f\u548c\u547d\u4ee4\u884c\u914d\u7f6e\u53c2\u6570\u3002",source:"@site/versions/version-4.2.0/zh-CN/source/5.installation&running/7.tugraph-running.md",sourceDirName:"5.installation&running",slug:"/installation&running/tugraph-running",permalink:"/tugraph-db/en-US/zh/4.2.0/installation&running/tugraph-running",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:7,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"\u4ece\u6e90\u7801\u7f16\u8bd1",permalink:"/tugraph-db/en-US/zh/4.2.0/installation&running/compile"},next:{title:"\u90e8\u7f72\u9ad8\u53ef\u7528\u6a21\u5f0f",permalink:"/tugraph-db/en-US/zh/4.2.0/installation&running/high-availability-mode"}},h={},c=[{value:"1.\u524d\u7f6e\u6761\u4ef6",id:"1\u524d\u7f6e\u6761\u4ef6",level:2},{value:"2.\u8fd0\u884c\u6a21\u5f0f",id:"2\u8fd0\u884c\u6a21\u5f0f",level:2},{value:"2.1.\u8fd0\u884c\u666e\u901a\u8fdb\u7a0b",id:"21\u8fd0\u884c\u666e\u901a\u8fdb\u7a0b",level:3},{value:"2.2.\u8fd0\u884c\u8fdb\u7a0b\u5b88\u62a4\u6a21\u5f0f",id:"22\u8fd0\u884c\u8fdb\u7a0b\u5b88\u62a4\u6a21\u5f0f",level:3},{value:"3.\u670d\u52a1\u64cd\u4f5c",id:"3\u670d\u52a1\u64cd\u4f5c",level:2},{value:"3.1.\u542f\u52a8\u670d\u52a1",id:"31\u542f\u52a8\u670d\u52a1",level:3},{value:"3.2.\u505c\u6b62\u670d\u52a1",id:"32\u505c\u6b62\u670d\u52a1",level:3},{value:"3.3.\u91cd\u542f\u670d\u52a1",id:"33\u91cd\u542f\u670d\u52a1",level:3},{value:"4.\u670d\u52a1\u914d\u7f6e",id:"4\u670d\u52a1\u914d\u7f6e",level:2},{value:"4.1.\u914d\u7f6e\u53c2\u6570",id:"41\u914d\u7f6e\u53c2\u6570",level:3},{value:"4.2.\u670d\u52a1\u5668\u914d\u7f6e\u6587\u4ef6",id:"42\u670d\u52a1\u5668\u914d\u7f6e\u6587\u4ef6",level:3}];function a(n){const e={a:"a",blockquote:"blockquote",code:"code",em:"em",h1:"h1",h2:"h2",h3:"h3",header:"header",nobr:"nobr",p:"p",pre:"pre",strong:"strong",table:"table",tbody:"tbody",td:"td",th:"th",thead:"thead",tr:"tr",...(0,d.R)(),...n.components};return(0,s.jsxs)(s.Fragment,{children:[(0,s.jsx)(e.header,{children:(0,s.jsx)(e.h1,{id:"\u6570\u636e\u5e93\u8fd0\u884c",children:"\u6570\u636e\u5e93\u8fd0\u884c"})}),"\n",(0,s.jsxs)(e.blockquote,{children:["\n",(0,s.jsx)(e.p,{children:"\u672c\u6587\u6863\u4e3b\u8981\u63cf\u8ff0 TuGraph \u670d\u52a1\u7684\u8fd0\u884c\u6a21\u5f0f\u3001\u542f\u52a8\u3001\u505c\u6b62\u548c\u91cd\u542f\u7684\u64cd\u4f5c,\u4ee5\u53ca TuGraph \u7684\u670d\u52a1\u914d\u7f6e\u53c2\u6570\u3001\u914d\u7f6e\u6587\u4ef6\u683c\u5f0f\u548c\u547d\u4ee4\u884c\u914d\u7f6e\u53c2\u6570\u3002"}),"\n"]}),"\n",(0,s.jsx)(e.h2,{id:"1\u524d\u7f6e\u6761\u4ef6",children:"1.\u524d\u7f6e\u6761\u4ef6"}),"\n",(0,s.jsxs)(e.p,{children:["TuGraph \u8fd0\u884c\u7684\u524d\u7f6e\u6761\u4ef6\u4e3a TuGraph \u6b63\u786e\u5b89\u88c5\uff0c\u53c2\u8003",(0,s.jsx)(e.a,{href:"/tugraph-db/en-US/zh/4.2.0/installation&running/environment",children:"\u5b89\u88c5\u6d41\u7a0b"}),"\u3002"]}),"\n",(0,s.jsx)(e.p,{children:"TuGraph \u8fd0\u884c\u9700\u8981\u4fdd\u8bc1\u5e93\u6587\u4ef6 liblgraph.so \u7684\u6587\u4ef6\u4f4d\u7f6e\u5728\u73af\u5883\u53d8\u91cf LD_LIBRARY_PATH\u3002"}),"\n",(0,s.jsx)(e.p,{children:"\u8fd0\u884c TuGraph \u8fdb\u7a0b\u7684\u7528\u6237\u4e0d\u9700\u8981\u8d85\u7ea7\u6743\u9650\uff0c\u4f46\u9700\u8981\u5bf9\u914d\u7f6e\u6587\u4ef6\uff08\u4e00\u822c\u4e3algraph.json\uff09\u53ca\u6587\u4ef6\u4e2d\u6d89\u53ca\u7684\u6587\u4ef6\u6709\u8bfb\u6743\u9650\uff0c\u5e76\u4e14\u5bf9\u6570\u636e\u6587\u4ef6\u5939\u3001\u65e5\u5fd7\u6587\u4ef6\u5939\u7b49\u6709\u5199\u6743\u9650\u3002"}),"\n",(0,s.jsx)(e.h2,{id:"2\u8fd0\u884c\u6a21\u5f0f",children:"2.\u8fd0\u884c\u6a21\u5f0f"}),"\n",(0,s.jsx)(e.p,{children:"TuGraph \u53ef\u4ee5\u4f5c\u4e3a\u524d\u53f0\u666e\u901a\u8fdb\u7a0b\u542f\u52a8\uff0c\u4e5f\u53ef\u4ee5\u4f5c\u4e3a\u540e\u53f0\u5b88\u62a4\u8fdb\u7a0b\u542f\u52a8\u3002\n\u5f53\u4f5c\u4e3a\u666e\u901a\u8fdb\u7a0b\u8fd0\u884c\u65f6\uff0cTuGraph \u53ef\u4ee5\u76f4\u63a5\u5c06\u65e5\u5fd7\u6253\u5370\u5230\u7ec8\u7aef\uff0c\u8fd9\u5728\u8c03\u8bd5\u670d\u52a1\u5668\u914d\u7f6e\u65f6\u975e\u5e38\u65b9\u4fbf\u3002\u4f46\u662f\uff0c\u7531\u4e8e\u524d\u53f0\u8fdb\u7a0b\u5728\u7ec8\u7aef\u9000\u51fa\u540e\u88ab\u7ec8\u6b62\uff0c\u56e0\u6b64\u7528\u6237\u987b\u786e\u4fdd\u5728 TuGraph \u670d\u52a1\u5668\u5904\u4e8e\u8fd0\u884c\u72b6\u6001\u65f6\uff0c\u7ec8\u7aef\u4fdd\u6301\u6253\u5f00\u72b6\u6001\u3002\u53e6\u4e00\u65b9\u9762\uff0c\u5728\u5b88\u62a4\u8fdb\u7a0b\u6a21\u5f0f\u4e0b\uff0c\u5373\u4f7f\u542f\u52a8\u5b83\u7684\u7ec8\u7aef\u9000\u51fa\uff0cTuGraph \u670d\u52a1\u5668\u4e5f\u53ef\u4ee5\u7ee7\u7eed\u8fd0\u884c\u3002\u56e0\u6b64\uff0c\u5728\u957f\u65f6\u95f4\u8fd0\u884c\u7684\u670d\u52a1\u5668\u4e0b\u63a8\u8350\u4ee5\u5b88\u62a4\u8fdb\u7a0b\u6a21\u5f0f\u542f\u52a8 TuGraph \u670d\u52a1\u5668\u3002"}),"\n",(0,s.jsx)(e.h3,{id:"21\u8fd0\u884c\u666e\u901a\u8fdb\u7a0b",children:"2.1.\u8fd0\u884c\u666e\u901a\u8fdb\u7a0b"}),"\n",(0,s.jsxs)(e.p,{children:[(0,s.jsx)(e.code,{children:"lgraph_server -d run"}),"\u547d\u4ee4\u53ef\u4ee5\u5c06 TuGraph \u4f5c\u4e3a\u666e\u901a\u8fdb\u7a0b\u8fd0\u884c\u3002\u666e\u901a\u8fdb\u7a0b\u4f9d\u8d56\u547d\u4ee4\u884c\u7ec8\u7aef\uff0c\u56e0\u6b64\u7ec8\u7aef\u7ed3\u675f\u65f6\uff0cTuGraph \u8fdb\u7a0b\u4e5f\u4f1a\u81ea\u52a8\u7ec8\u6b62\u3002\u666e\u901a\u8fdb\u7a0b\u6a21\u5f0f\u914d\u5408",(0,s.jsx)(e.code,{children:'--log_dir ""'}),"\u53ef\u4ee5\u5c06\u8fdb\u7a0b\u65e5\u5fd7\u76f4\u63a5\u8f93\u51fa\u5230\u7ec8\u7aef\uff0c\u56e0\u6b64\u66f4\u65b9\u4fbf\u8c03\u8bd5\u3002\u6ce8\uff1a\u5f53\u4e0d\u4f7f\u7528",(0,s.jsx)(e.code,{children:"-d run"}),"\u547d\u4ee4\u65f6\uff0c\u5c06\u9ed8\u8ba4\u8fd0\u884c\u666e\u901a\u8fdb\u7a0b\u3002"]}),"\n",(0,s.jsx)(e.p,{children:"\u542f\u52a8\u547d\u4ee4\uff1a"}),"\n",(0,s.jsx)(e.pre,{children:(0,s.jsx)(e.code,{className:"language-shell",children:'$ ./lgraph_server -d run -c lgraph_standalone.json --log_dir ""\n'})}),"\n",(0,s.jsx)(e.p,{children:"\u6216\u8005\uff1a"}),"\n",(0,s.jsx)(e.pre,{children:(0,s.jsx)(e.code,{className:"language-shell",children:'$ ./lgraph_server -c lgraph_standalone.json --log_dir ""\n'})}),"\n",(0,s.jsx)(e.p,{children:"\u666e\u901a\u6a21\u5f0f\u7684\u8fd0\u884c\u8f93\u51fa\u793a\u4f8b\uff1a"}),"\n",(0,s.jsx)(e.pre,{children:(0,s.jsx)(e.code,{className:"language-shell",children:'**********************************************************************\n*                  TuGraph Graph Database v3.6.0                     *\n*                                                                    *\n*    Copyright(C) 2018-2023 Ant Group. All rights reserved.          *\n*                                                                    *\n**********************************************************************\nServer is configured with the following parameters:\n  Backup log enable:               0\n  DB directory:                    ./lgraph_db\n  HA enable:                       0\n  HTTP port:                       7071\n  HTTP web dir:                    ./resource\n  RPC enable:                      1\n  RPC port:                        9091\n  SSL enable:                      0\n  Whether the token is unlimited:  0\n  audit log enable:                0\n  bind host:                       0.0.0.0\n  disable auth:                    0\n  durable:                         0\n  log dir:                         ""\n  log verbose:                     2\n  optimistic transaction:          0\n  subprocess idle limit:           600\n  thread limit:                    0\n[2023-Aug-23 15:35:29.172716] [INFO] - [StateMachine] Builtin services are disabled according to ServerOptions.has_builtin_services\n[2023-Aug-23 15:35:29.174881] [INFO] - Listening for RPC on port 9091\n[2023-Aug-23 15:35:29.176401] [DEBUG] [Galaxy] - Loading DB state from disk\n[2023-Aug-23 15:35:29.185353] [INFO] [RestServer] - Listening for REST on port 7071\n[2023-Aug-23 15:35:29.185585] [INFO] - Server started.\n'})}),"\n",(0,s.jsxs)(e.p,{children:["\u666e\u901a\u8fdb\u7a0b\u6a21\u5f0f\u4e0b\uff0c\u7528\u6237\u53ef\u4ee5\u901a\u8fc7\u6309 ",(0,s.jsx)(e.code,{children:"CTRL+C"})," \u6765\u63d0\u524d\u7ec8\u6b62 TuGraph \u8fdb\u7a0b\u3002"]}),"\n",(0,s.jsx)(e.h3,{id:"22\u8fd0\u884c\u8fdb\u7a0b\u5b88\u62a4\u6a21\u5f0f",children:"2.2.\u8fd0\u884c\u8fdb\u7a0b\u5b88\u62a4\u6a21\u5f0f"}),"\n",(0,s.jsx)(e.p,{children:"\u542f\u52a8\u547d\u4ee4\uff1a"}),"\n",(0,s.jsx)(e.pre,{children:(0,s.jsx)(e.code,{className:"language-shell",children:"$ ./lgraph_server -d start -c lgraph_daemon.json\n"})}),"\n",(0,s.jsx)(e.p,{children:"\u5b88\u62a4\u6a21\u5f0f\u7684\u8fd0\u884c\u8f93\u51fa\u793a\u4f8b\uff1a"}),"\n",(0,s.jsx)(e.pre,{children:(0,s.jsx)(e.code,{className:"language-shell",children:"Starting lgraph...\nThe service process is started at pid 12109.\n"})}),"\n",(0,s.jsxs)(e.p,{children:["\u6b64\u547d\u4ee4\u542f\u52a8\u7684 TuGraph \u670d\u52a1\u5668\u8fdb\u7a0b\u4e3a\u5b88\u62a4\u8fdb\u7a0b\uff0c\u5b83\u5c06\u4ece\u6587\u4ef6",(0,s.jsx)(e.code,{children:"lgraph_daemon.json"}),"\u52a0\u8f7d\u76f8\u5173\u914d\u7f6e\u3002\u670d\u52a1\u5668\u542f\u52a8\u540e\uff0c\u5b83\u5c06\u5f00\u59cb\u5728\u65e5\u5fd7\u6587\u4ef6\u4e2d\u6253\u5370\u65e5\u5fd7\uff0c\u4e4b\u540e\u53ef\u7528\u8be5\u65e5\u5fd7\u6587\u4ef6\u786e\u5b9a\u670d\u52a1\u5668\u7684\u72b6\u6001\u3002"]}),"\n",(0,s.jsx)(e.h2,{id:"3\u670d\u52a1\u64cd\u4f5c",children:"3.\u670d\u52a1\u64cd\u4f5c"}),"\n",(0,s.jsx)(e.h3,{id:"31\u542f\u52a8\u670d\u52a1",children:"3.1.\u542f\u52a8\u670d\u52a1"}),"\n",(0,s.jsxs)(e.p,{children:["TuGraph \u9700\u8981\u901a\u8fc7 ",(0,s.jsx)(e.code,{children:"lgraph_server -d start"})," \u547d\u4ee4\u884c\u542f\u52a8\uff0c\u542f\u52a8\u547d\u4ee4\u793a\u4f8b\u5982\u4e0b\uff1a"]}),"\n",(0,s.jsx)(e.pre,{children:(0,s.jsx)(e.code,{className:"language-bash",children:"$ ./lgraph_server -d start -c lgraph_daemon.json\nStarting lgraph...\nThe service process is started at pid 12109.\n"})}),"\n",(0,s.jsxs)(e.p,{children:["\u6b64\u547d\u4ee4\u542f\u52a8\u7684 TuGraph \u670d\u52a1\u5668\u8fdb\u7a0b\u4e3a\u5b88\u62a4\u8fdb\u7a0b\uff0c\u5b83\u5c06\u4ece\u6587\u4ef6",(0,s.jsx)(e.code,{children:"lgraph_daemon.json"}),"\u52a0\u8f7d\u76f8\u5173\u914d\u7f6e\u3002\u670d\u52a1\u5668\u542f\u52a8\u540e\uff0c\u5b83\u5c06\u5f00\u59cb\u5728\u65e5\u5fd7\u6587\u4ef6\u4e2d\u6253\u5370\u65e5\u5fd7\uff0c\u4e4b\u540e\u53ef\u7528\u8be5\u65e5\u5fd7\u6587\u4ef6\u786e\u5b9a\u670d\u52a1\u5668\u7684\u72b6\u6001\u3002"]}),"\n",(0,s.jsx)(e.h3,{id:"32\u505c\u6b62\u670d\u52a1",children:"3.2.\u505c\u6b62\u670d\u52a1"}),"\n",(0,s.jsxs)(e.p,{children:["\u7528\u6237\u53ef\u4ee5\u4f7f\u7528",(0,s.jsx)(e.code,{children:"kill"}),"\u547d\u4ee4\u4ee5\u53ca",(0,s.jsx)(e.code,{children:"lgraph_server -d stop"}),"\u547d\u4ee4\u505c\u6b62 TuGraph \u5b88\u62a4\u8fdb\u7a0b\u3002\u7531\u4e8e\u53ef\u80fd\u5728\u540c\u4e00\u53f0\u8ba1\u7b97\u673a\u4e0a\u8fd0\u884c\u591a\u4e2a TuGraph \u670d\u52a1\u5668\u8fdb\u7a0b\uff0c\u56e0\u6b64\u6211\u4eec\u4f7f\u7528",(0,s.jsx)(e.code,{children:".pid"}),"\u6587\u4ef6\u533a\u5206\u4e0d\u540c\u7684\u670d\u52a1\u5668\u8fdb\u7a0b\uff0c\u8be5\u6587\u4ef6\u5199\u5165\u542f\u52a8\u8be5\u8fdb\u7a0b\u7684\u5de5\u4f5c\u76ee\u5f55\u3002\u56e0\u6b64\uff0c\u9700\u8981\u5728\u76f8\u540c\u5de5\u4f5c\u76ee\u5f55\u4e2d\u8fd0\u884c",(0,s.jsx)(e.code,{children:"lgraph_server-d stop"}),"\u547d\u4ee4\uff0c\u4ee5\u505c\u6b62\u6b63\u786e\u7684\u670d\u52a1\u5668\u8fdb\u7a0b\u3002"]}),"\n",(0,s.jsx)(e.pre,{children:(0,s.jsx)(e.code,{className:"language-shell",children:"user@host:~/tugraph$ ./lgraph_server -d start -c lgraph_standalone.json\n20200508122306.378: Starting lgraph...\n20200508122306.379: The service process is started at pid 93.\n\nuser@host:~/tugraph$ cat ./lgraph.pid\n93\n\nuser@host:~/tugraph$ ./lgraph_server -d stop -c lgraph_standalone.json\n20200508122334.857: Stopping lgraph...\n20200508122334.857: Process stopped.\n"})}),"\n",(0,s.jsx)(e.h3,{id:"33\u91cd\u542f\u670d\u52a1",children:"3.3.\u91cd\u542f\u670d\u52a1"}),"\n",(0,s.jsxs)(e.p,{children:["\u7528\u6237\u4e5f\u53ef\u4ee5\u901a\u8fc7",(0,s.jsx)(e.code,{children:"lgraph_server -d restart"}),"\u6765\u91cd\u542f TuGraph \u670d\u52a1\uff1a"]}),"\n",(0,s.jsx)(e.pre,{children:(0,s.jsx)(e.code,{className:"language-bash",children:"$ ./lgraph_server -d restart\nStopping lgraph...\nProcess stopped.\nStarting lgraph...\nThe service process is started at pid 20899.\n"})}),"\n",(0,s.jsx)(e.h2,{id:"4\u670d\u52a1\u914d\u7f6e",children:"4.\u670d\u52a1\u914d\u7f6e"}),"\n",(0,s.jsx)(e.p,{children:"TuGraph \u670d\u52a1\u5668\u5728\u542f\u52a8\u65f6\u4ece\u914d\u7f6e\u6587\u4ef6\u548c\u547d\u4ee4\u884c\u9009\u9879\u52a0\u8f7d\u914d\u7f6e\uff0c\u5982\u679c\u5728\u914d\u7f6e\u6587\u4ef6\u548c\u547d\u4ee4\u884c\u4e2d\u540c\u4e00\u9009\u9879\u6307\u5b9a\u4e86\u4e0d\u540c\u7684\u503c\uff0c\u5c06\u4f18\u5148\u4f7f\u7528\u547d\u4ee4\u884c\u4e2d\u6307\u5b9a\u7684\u503c\u3002"}),"\n",(0,s.jsx)(e.h3,{id:"41\u914d\u7f6e\u53c2\u6570",children:"4.1.\u914d\u7f6e\u53c2\u6570"}),"\n",(0,s.jsx)(e.p,{children:"\u5177\u4f53\u53c2\u6570\u53ca\u5176\u7c7b\u578b\u63cf\u8ff0\u5982\u4e0b\uff1a"}),"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",(0,s.jsxs)(e.table,{children:[(0,s.jsx)(e.thead,{children:(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.th,{children:(0,s.jsx)(e.strong,{children:"\u53c2\u6570\u540d"})}),(0,s.jsx)(e.th,{children:(0,s.jsx)(e.strong,{children:(0,s.jsx)(e.nobr,{children:"\u53c2\u6570\u7c7b\u578b"})})}),(0,s.jsx)(e.th,{children:(0,s.jsx)(e.strong,{children:"\u53c2\u6570\u8bf4\u660e"})})]})}),(0,s.jsxs)(e.tbody,{children:[(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"directory"}),(0,s.jsx)(e.td,{children:"\u5b57\u7b26\u4e32"}),(0,s.jsx)(e.td,{children:"\u6570\u636e\u6587\u4ef6\u6240\u5728\u76ee\u5f55\u3002\u5982\u679c\u76ee\u5f55\u4e0d\u5b58\u5728 \uff0c\u5219\u81ea\u52a8\u521b\u5efa\u3002\u9ed8\u8ba4\u76ee\u5f55\u4e3a /var/lib/lgraph/data\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"durable"}),(0,s.jsx)(e.td,{children:"\u5e03\u5c14\u503c"}),(0,s.jsxs)(e.td,{children:["\u662f\u5426\u5f00\u542f\u5b9e\u65f6\u6301\u4e45\u5316\u3002\u5173\u95ed\u6301\u4e45\u5316\u53ef\u4ee5\u51cf\u5c11\u5199\u5165\u65f6\u7684\u78c1\u76d8 IO \u5f00\u9500\uff0c\u4f46\u662f\u5728\u673a\u5668\u65ad\u7535\u7b49\u6781\u7aef\u60c5\u51b5\u4e0b\u53ef\u80fd\u4e22\u5931\u6570\u636e\u3002\u9ed8\u8ba4\u503c\u4e3a ",(0,s.jsx)(e.code,{children:"true"}),"\u3002"]})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"host"}),(0,s.jsx)(e.td,{children:"\u5b57\u7b26\u4e32"}),(0,s.jsx)(e.td,{children:"REST \u670d\u52a1\u5668\u76d1\u542c\u65f6\u4f7f\u7528\u7684\u5730\u5740\uff0c\u4e00\u822c\u4e3a\u670d\u52a1\u5668\u7684 IP \u5730\u5740\u3002\u9ed8\u8ba4\u5730\u5740\u4e3a 0.0.0.0\u3002\u6ce8\uff1a\u5728HA\u6a21\u5f0f\u4e0b\uff0chost\u9700\u8981\u8bbe\u7f6e\u4e3a\u5bf9\u5e94\u670d\u52a1\u5668\u7684IP\u5730\u5740\uff0c\u4e0d\u80fd\u8bbe\u7f6e\u4e3a0.0.0.0\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"port"}),(0,s.jsx)(e.td,{children:"\u6574\u578b"}),(0,s.jsx)(e.td,{children:"REST \u670d\u52a1\u5668\u76d1\u542c\u65f6\u4f7f\u7528\u7684\u7aef\u53e3\u3002\u9ed8\u8ba4\u7aef\u53e3\u4e3a 7070\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"enable_rpc"}),(0,s.jsx)(e.td,{children:"\u5e03\u5c14\u503c"}),(0,s.jsx)(e.td,{children:"\u662f\u5426\u4f7f\u7528 RPC \u670d\u52a1\u3002\u9ed8\u8ba4\u503c\u4e3a false\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"rpc_port"}),(0,s.jsx)(e.td,{children:"\u6574\u578b"}),(0,s.jsx)(e.td,{children:"RPC \u53ca HA \u670d\u52a1\u6240\u7528\u7aef\u53e3\u3002\u9ed8\u8ba4\u7aef\u53e3\u4e3a 9090\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"enable_ha"}),(0,s.jsx)(e.td,{children:"\u5e03\u5c14\u503c"}),(0,s.jsx)(e.td,{children:"\u662f\u5426\u542f\u52a8\u9ad8\u53ef\u7528\u6a21\u5f0f\u3002\u9ed8\u8ba4\u503c\u4e3a false\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"ha_log_dir"}),(0,s.jsx)(e.td,{children:"\u5b57\u7b26\u4e32"}),(0,s.jsx)(e.td,{children:"HA \u65e5\u5fd7\u6240\u5728\u76ee\u5f55\uff0c\u9700\u8981\u542f\u52a8 HA \u6a21\u5f0f\u3002\u9ed8\u8ba4\u503c\u4e3a\u7a7a\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"verbose"}),(0,s.jsx)(e.td,{children:"\u6574\u578b"}),(0,s.jsx)(e.td,{children:"\u65e5\u5fd7\u8f93\u51fa\u4fe1\u606f\u7684\u8be6\u7ec6\u7a0b\u5ea6\u3002\u53ef\u8bbe\u4e3a 0\uff0c1\uff0c2\uff0c\u503c\u8d8a\u5927\u5219\u8f93\u51fa\u4fe1\u606f\u8d8a\u8be6\u7ec6\u3002\u9ed8\u8ba4\u503c\u4e3a 1\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"log_dir"}),(0,s.jsx)(e.td,{children:"\u5b57\u7b26\u4e32"}),(0,s.jsx)(e.td,{children:"\u65e5\u5fd7\u6587\u4ef6\u6240\u5728\u7684\u76ee\u5f55\u3002\u9ed8\u8ba4\u76ee\u5f55\u4e3a /var/log/lgraph/\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"ssl_auth"}),(0,s.jsx)(e.td,{children:"\u5e03\u5c14\u503c"}),(0,s.jsx)(e.td,{children:"\u662f\u5426\u4f7f\u7528 SSL \u5b89\u5168\u8ba4\u8bc1\u3002\u5f53\u5f00\u542f\u65f6\uff0cREST \u670d\u52a1\u5668\u53ea\u5f00\u542f HTTPS \u670d\u52a1\u3002\u9ed8\u8ba4\u503c\u4e3a false\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"web"}),(0,s.jsx)(e.td,{children:"\u5b57\u7b26\u4e32"}),(0,s.jsx)(e.td,{children:"web \u6587\u4ef6\uff08\u5305\u542b\u53ef\u89c6\u5316\u90e8\u5206\uff09\u6240\u5728\u76ee\u5f55\u3002\u9ed8\u8ba4\u76ee\u5f55\u4e3a /usr/local/share/lgraph/resource\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"server_cert"}),(0,s.jsx)(e.td,{children:"\u5b57\u7b26\u4e32"}),(0,s.jsx)(e.td,{children:"\u5728 SSL \u8ba4\u8bc1\u5f00\u542f\u65f6\uff0c\u670d\u52a1\u5668\u6240\u4f7f\u7528\u7684 certificate \u6587\u4ef6\u8def\u5f84\u3002\u9ed8\u8ba4\u8def\u5f84\u4e3a /usr/local/etc/lgraph/server-cert.pem\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"server_key"}),(0,s.jsx)(e.td,{children:"\u5b57\u7b26\u4e32"}),(0,s.jsx)(e.td,{children:"\u5728 SSL \u8ba4\u8bc1\u5f00\u542f\u65f6\uff0c\u670d\u52a1\u5668\u6240\u4f7f\u7528\u7684\u516c\u94a5\u6587\u4ef6\u3002\u9ed8\u8ba4\u76ee\u5f55\u4e3a /usr/local/etc/lgraph/server-key.pem\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"enable_audit_log"}),(0,s.jsx)(e.td,{children:"\u5e03\u5c14\u503c"}),(0,s.jsx)(e.td,{children:"\u662f\u5426\u542f\u7528\u5ba1\u8ba1\u65e5\u5fd7\uff0c\u9ed8\u8ba4\u503c\u4e3a false\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"audit_log_expire"}),(0,s.jsx)(e.td,{children:"\u6574\u578b"}),(0,s.jsx)(e.td,{children:"\u542f\u7528\u5ba1\u8ba1\u65e5\u5fd7\u65f6\uff0c\u65e5\u5fd7\u7684\u6709\u6548\u65f6\u95f4\uff08\u5c0f\u65f6\uff09\uff0c\u8d85\u65f6\u81ea\u52a8\u6e05\u7406\uff0c\u503c\u4e3a 0 \u65f6\u8868\u793a\u4e0d\u6e05\u7406\u3002\u9ed8\u8ba4\u503c\u4e3a 0\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"audit_log_dir"}),(0,s.jsx)(e.td,{children:"\u5b57\u7b26\u4e32"}),(0,s.jsxs)(e.td,{children:["\u542f\u7528\u5ba1\u8ba1\u65e5\u5fd7\u65f6\uff0c\u65e5\u5fd7\u6587\u4ef6\u7684\u5b58\u653e\u76ee\u5f55\u3002\u9ed8\u8ba4\u76ee\u5f55\u4e3a $directory/",(0,s.jsx)(e.em,{children:"audit_log"}),"\u3002"]})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"load_plugins"}),(0,s.jsx)(e.td,{children:"\u5e03\u5c14\u503c"}),(0,s.jsx)(e.td,{children:"\u542f\u52a8\u670d\u52a1\u65f6\u5bfc\u5165\u6240\u6709\u5b58\u50a8\u8fc7\u7a0b\u3002\u9ed8\u8ba4\u503c\u4e3a true\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"optimistic_txn"}),(0,s.jsx)(e.td,{children:"\u5e03\u5c14\u503c"}),(0,s.jsx)(e.td,{children:"\u4e3a Cypher \u5f00\u542f\u4e50\u89c2\u591a\u7ebf\u7a0b\u5199\u5165\u4e8b\u52a1\u3002\u9ed8\u8ba4\u4e3a false\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"disable_auth"}),(0,s.jsx)(e.td,{children:"\u5e03\u5c14\u503c"}),(0,s.jsx)(e.td,{children:"\u5173\u95ed REST \u9a8c\u8bc1\u3002\u9ed8\u8ba4\u4e3a false\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"ha_snapshot_interval_s"}),(0,s.jsx)(e.td,{children:"\u6574\u578b"}),(0,s.jsx)(e.td,{children:"\u5feb\u7167\u95f4\u9694\uff08\u4ee5\u79d2\u4e3a\u5355\u4f4d\uff09\uff0c\u9ed8\u8ba4\u503c\u4e3a 604800\u3002-1\u8868\u793a\u4e0d\u81ea\u52a8\u751f\u6210\u5feb\u7167\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"ha_heartbeat_interval_ms"}),(0,s.jsx)(e.td,{children:"\u6574\u578b"}),(0,s.jsx)(e.td,{children:"\u5fc3\u8df3\u95f4\u9694\uff08\u4ee5\u6beb\u79d2\u4e3a\u5355\u4f4d\uff09\u3002 \u9ed8\u8ba4\u503c\u4e3a 1000\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"ha_node_offline_ms"}),(0,s.jsx)(e.td,{children:"\u6574\u578b"}),(0,s.jsx)(e.td,{children:"\u5fc3\u8df3\u8d85\u65f6\u4e14\u8282\u70b9\u4e0b\u7ebf\u95f4\u9694\uff08\u4ee5\u6beb\u79d2\u4e3a\u5355\u4f4d\uff09\u3002\u9ed8\u8ba4\u4e3a 60000\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"ha_node_remove_ms"}),(0,s.jsx)(e.td,{children:"\u6574\u578b"}),(0,s.jsx)(e.td,{children:"\u8282\u70b9\u88ab\u89c6\u4e3a\u5b8c\u5168\u6b7b\u4ea1\u5e76\u4ece\u5217\u8868\u4e2d\u5220\u9664\u7684\u95f4\u9694\uff08\u4ee5\u6beb\u79d2\u4e3a\u5355\u4f4d\uff09\u3002\u9ed8\u8ba4\u503c\u4e3a 120000\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"enable_ip_check"}),(0,s.jsx)(e.td,{children:"\u5e03\u5c14\u503c"}),(0,s.jsx)(e.td,{children:"\u5141\u8bb8 IP \u767d\u540d\u5355\uff0c\u9ed8\u8ba4\u503c\u4e3a false\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"idle_seconds"}),(0,s.jsx)(e.td,{children:"\u6574\u578b"}),(0,s.jsx)(e.td,{children:"\u5b50\u8fdb\u7a0b\u53ef\u4ee5\u5904\u4e8e\u7a7a\u95f2\u72b6\u6001\u7684\u6700\u5927\u79d2\u6570\u3002 \u9ed8\u8ba4\u503c\u4e3a 600\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"enable_backup_log"}),(0,s.jsx)(e.td,{children:"\u5e03\u5c14\u503c"}),(0,s.jsx)(e.td,{children:"\u662f\u5426\u542f\u7528\u5907\u4efd\u65e5\u5fd7\u8bb0\u5f55\u3002 \u9ed8\u8ba4\u503c\u4e3a false\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"backup_log_dir"}),(0,s.jsx)(e.td,{children:"\u5b57\u7b26\u4e32"}),(0,s.jsx)(e.td,{children:"\u5b58\u50a8\u5907\u4efd\u6587\u4ef6\u7684\u76ee\u5f55\u3002 \u9ed8\u8ba4\u503c\u4e3a\u7a7a\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"snapshot_dir"}),(0,s.jsx)(e.td,{children:"\u5b57\u7b26\u4e32"}),(0,s.jsx)(e.td,{children:"\u5b58\u50a8\u5feb\u7167\u6587\u4ef6\u7684\u76ee\u5f55\u3002 \u9ed8\u8ba4\u503c\u4e3a\u7a7a\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"thread_limit"}),(0,s.jsx)(e.td,{children:"\u6574\u578b"}),(0,s.jsx)(e.td,{children:"\u540c\u65f6\u4f7f\u7528\u7684\u6700\u5927\u7ebf\u7a0b\u6570\u3002 \u9ed8\u8ba4\u503c\u4e3a 0\uff0c\u5373\u4e0d\u505a\u9650\u5236\uff0c\u4ee5 license \u4e3a\u51c6\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"unlimited_token"}),(0,s.jsx)(e.td,{children:"\u5e03\u5c14\u503c"}),(0,s.jsx)(e.td,{children:"\u662f\u5426\u5c06\u94fe\u63a5token\u8bbe\u7f6e\u4e3a\u65e0\u671f\u9650\u3002 \u9ed8\u8ba4\u503c\u4e3a false\uff0c\u6709\u6548\u671f\u4e3a24\u5c0f\u65f6\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"reset_admin_password"}),(0,s.jsx)(e.td,{children:"\u5e03\u5c14\u503c"}),(0,s.jsxs)(e.td,{children:["\u662f\u5426\u91cd\u7f6e\u7ba1\u7406\u8005\u5bc6\u7801\u3002 \u9ed8\u8ba4\u503c\u4e3a false\u3002 \u4e3a true \u65f6\uff0c\u5bc6\u7801\u91cd\u7f6e\u4e3a",(0,s.jsx)(e.code,{children:"73@TuGraph"}),"\u3002"]})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"enable_fulltext_index"}),(0,s.jsx)(e.td,{children:"\u5e03\u5c14\u503c"}),(0,s.jsx)(e.td,{children:"\u662f\u5426\u542f\u7528\u5168\u6587\u7d22\u5f15\u529f\u80fd\uff0c\u9ed8\u8ba4\u503c\u4e3a false\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"fulltext_analyzer"}),(0,s.jsx)(e.td,{children:"\u5b57\u7b26\u4e32"}),(0,s.jsxs)(e.td,{children:["\u5168\u6587\u7d22\u5f15\u5206\u8bcd\u5668\u7c7b\u578b\u3002\u53ef\u8bbe\u4e3a",(0,s.jsx)(e.code,{children:"StandardAnalyzer"}),"\u6216\u8005",(0,s.jsx)(e.code,{children:"SmartChineseAnalyzer"}),"\u3002\u9ed8\u8ba4\u662f",(0,s.jsx)(e.code,{children:"StandardAnalyzer"})]})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"fulltext_commit_interval"}),(0,s.jsx)(e.td,{children:"\u6574\u5f62"}),(0,s.jsx)(e.td,{children:"\u5168\u6587\u7d22\u5f15\u6570\u636e\u63d0\u4ea4\u5468\u671f,\u9488\u5bf9\u5199\u64cd\u4f5c\uff0c\u5355\u4f4d\u79d2\u3002\u9ed8\u8ba4\u662f 0\uff0c\u7acb\u5373\u63d0\u4ea4\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"fulltext_refresh_interval"}),(0,s.jsx)(e.td,{children:"\u6574\u5f62"}),(0,s.jsx)(e.td,{children:"\u5168\u6587\u7d22\u5f15\u6570\u636e\u5237\u65b0\u5468\u671f\uff0c\u9488\u5bf9\u8bfb\u64cd\u4f5c\uff0c\u5355\u4f4d\u79d2\u3002\u9ed8\u8ba4\u662f 0\uff0c\u7acb\u5373\u53ef\u4ee5\u8bfb\u5230\u6700\u65b0\u5199\u5165\u7684\u6570\u636e\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"ha_conf"}),(0,s.jsx)(e.td,{children:"\u5b57\u7b26\u4e32"}),(0,s.jsx)(e.td,{children:"\u6839\u636e host1:port1,host2:port2,host3:port3 \u521d\u59cb\u5316HA\u96c6\u7fa4\uff0c\u9ed8\u8ba4\u503c\u4e3a\u7a7a\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"ha_node_join_group_s"}),(0,s.jsx)(e.td,{children:"\u6574\u5f62"}),(0,s.jsx)(e.td,{children:"\u8282\u70b9\u5c1d\u8bd5\u52a0\u5165\u9ad8\u53ef\u7528\u96c6\u7fa4\u7684\u7b49\u5f85\u65f6\u957f\uff0c\u5355\u4f4d\u79d2\uff0c\u9ed8\u8ba4\u662f 10\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"ha_bootstrap_role"}),(0,s.jsx)(e.td,{children:"\u6574\u5f62"}),(0,s.jsx)(e.td,{children:"\u662f\u5426\u4f7f\u7528bootstrap\u65b9\u5f0f\u542f\u52a8\uff0c\u4ee5\u53ca\u4f7f\u7528\u8be5\u65b9\u5f0f\u542f\u52a8\u7684\u670d\u52a1\u5668\u89d2\u8272\uff0c0\u4ee3\u8868\u4e0d\u4f7f\u7528bootstrap\u65b9\u5f0f\u542f\u52a8\uff0c1\u4ee3\u8868\u4f7f\u7528bootstrap\u65b9\u5f0f\u542f\u52a8\u4e14\u672c\u8282\u70b9\u4e3aleader\uff0c2\u4ee3\u8868\u4f7f\u7528bootstrap\u65b9\u5f0f\u542f\u52a8\u4e14\u672c\u8282\u70b9\u4e3afollower\uff0c\u53ea\u6709\u8fd93\u79cd\u9009\u9879\u3002 \u9ed8\u8ba4\u503c\u4e3a 0\u3002"})]}),(0,s.jsxs)(e.tr,{children:[(0,s.jsx)(e.td,{children:"help"}),(0,s.jsx)(e.td,{children:"\u5e03\u5c14\u503c"}),(0,s.jsx)(e.td,{children:"\u6253\u5370\u6b64\u5e2e\u52a9\u6d88\u606f\u3002 \u9ed8\u8ba4\u503c\u4e3a false\u3002"})]})]})]}),"\n",(0,s.jsx)(e.h3,{id:"42\u670d\u52a1\u5668\u914d\u7f6e\u6587\u4ef6",children:"4.2.\u670d\u52a1\u5668\u914d\u7f6e\u6587\u4ef6"}),"\n",(0,s.jsx)(e.p,{children:"TuGraph \u7684\u914d\u7f6e\u6587\u4ef6\u4ee5 JSON \u683c\u5f0f\u5b58\u50a8\u3002\u5efa\u8bae\u5c06\u5927\u591a\u6570\u914d\u7f6e\u5b58\u50a8\u5728\u914d\u7f6e\u6587\u4ef6\u4e2d\uff0c\u5e76\u4e14\u4ec5\u5728\u9700\u8981\u65f6\u4f7f\u7528\u547d\u4ee4\u884c\u9009\u9879\u4e34\u65f6\u4fee\u6539\u67d0\u4e9b\u914d\u7f6e\u53c2\u6570\u3002\n\u4e00\u4e2a\u5178\u578b\u7684\u914d\u7f6e\u6587\u4ef6\u5982\u4e0b\uff1a"}),"\n",(0,s.jsx)(e.pre,{children:(0,s.jsx)(e.code,{className:"language-json",children:'{\n  "directory": "/var/lib/lgraph/data",\n\n  "port": 7090,\n  "rpc_port": 9090,\n  "enable_ha": false,\n\n  "verbose": 1,\n  "log_dir": "/var/log/lgraph/",\n\n  "ssl_auth": false,\n  "server_key": "/usr/local/etc/lgraph/server-key.pem",\n  "server_cert": "/usr/local/etc/lgraph/server-cert.pem"\n}\n'})})]})}function x(n={}){const{wrapper:e}={...(0,d.R)(),...n.components};return e?(0,s.jsx)(e,{...n,children:(0,s.jsx)(a,{...n})}):a(n)}},28453:(n,e,r)=>{r.d(e,{R:()=>t,x:()=>i});var s=r(96540);const d={},l=s.createContext(d);function t(n){const e=s.useContext(l);return s.useMemo((function(){return"function"==typeof n?n(e):{...e,...n}}),[e,n])}function i(n){let e;return e=n.disableParentContext?"function"==typeof n.components?n.components(d):n.components||d:t(n.components),s.createElement(l.Provider,{value:e},n.children)}}}]);