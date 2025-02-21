"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[73871],{28453:(n,e,a)=>{a.d(e,{R:()=>i,x:()=>o});var r=a(96540);const t={},s=r.createContext(t);function i(n){const e=r.useContext(s);return r.useMemo((function(){return"function"==typeof n?n(e):{...e,...n}}),[e,n])}function o(n){let e;return e=n.disableParentContext?"function"==typeof n.components?n.components(t):n.components||t:i(n.components),r.createElement(s.Provider,{value:e},n.children)}},66467:(n,e,a)=>{a.d(e,{A:()=>r});const r=a.p+"assets/images/tugraph-datax-0f6f140ea310beb2c90460c3d6d0c08d.png"},84099:(n,e,a)=>{a.r(e),a.d(e,{assets:()=>l,contentTitle:()=>i,default:()=>p,frontMatter:()=>s,metadata:()=>o,toc:()=>d});var r=a(74848),t=a(28453);const s={},i="TuGraph-DataX",o={id:"developer-manual/ecosystem-tools/tugraph-datax",title:"TuGraph-DataX",description:"\u6b64\u6587\u6863\u4e3b\u8981\u4ecb\u7ecd TuGraph DataX \u7684\u5b89\u88c5\u7f16\u8bd1\u548c\u4f7f\u7528\u793a\u4f8b",source:"@site/versions/version-4.1.0/zh-CN/source/5.developer-manual/5.ecosystem-tools/2.tugraph-datax.md",sourceDirName:"5.developer-manual/5.ecosystem-tools",slug:"/developer-manual/ecosystem-tools/tugraph-datax",permalink:"/tugraph-db/en-US/zh/4.1.0/developer-manual/ecosystem-tools/tugraph-datax",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:2,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"\u8fd0\u7ef4\u76d1\u63a7",permalink:"/tugraph-db/en-US/zh/4.1.0/developer-manual/ecosystem-tools/monitoring"},next:{title:"TuGraph-Explorer",permalink:"/tugraph-db/en-US/zh/4.1.0/developer-manual/ecosystem-tools/tugraph-explorer"}},l={},d=[{value:"1.\u7b80\u4ecb",id:"1\u7b80\u4ecb",level:2},{value:"2.\u7f16\u8bd1\u5b89\u88c5",id:"2\u7f16\u8bd1\u5b89\u88c5",level:2},{value:"3.\u6587\u672c\u6570\u636e\u901a\u8fc7DataX\u5bfc\u5165TuGraph",id:"3\u6587\u672c\u6570\u636e\u901a\u8fc7datax\u5bfc\u5165tugraph",level:2},{value:"4.MySQL\u6570\u636e\u901a\u8fc7DataX\u5bfc\u5165TuGraph",id:"4mysql\u6570\u636e\u901a\u8fc7datax\u5bfc\u5165tugraph",level:2}];function c(n){const e={a:"a",blockquote:"blockquote",code:"code",h1:"h1",h2:"h2",header:"header",img:"img",li:"li",p:"p",pre:"pre",strong:"strong",ul:"ul",...(0,t.R)(),...n.components};return(0,r.jsxs)(r.Fragment,{children:[(0,r.jsx)(e.header,{children:(0,r.jsx)(e.h1,{id:"tugraph-datax",children:"TuGraph-DataX"})}),"\n",(0,r.jsxs)(e.blockquote,{children:["\n",(0,r.jsx)(e.p,{children:"\u6b64\u6587\u6863\u4e3b\u8981\u4ecb\u7ecd TuGraph DataX \u7684\u5b89\u88c5\u7f16\u8bd1\u548c\u4f7f\u7528\u793a\u4f8b"}),"\n"]}),"\n",(0,r.jsx)(e.h2,{id:"1\u7b80\u4ecb",children:"1.\u7b80\u4ecb"}),"\n",(0,r.jsxs)(e.p,{children:["TuGraph \u5728\u963f\u91cc\u5f00\u6e90\u7684 DataX \u57fa\u7840\u4e0a\u6dfb\u52a0\u4e86 TuGraph \u7684\u5199\u63d2\u4ef6\u4ee5\u53ca TuGraph jsonline \u6570\u636e\u683c\u5f0f\u7684\u652f\u6301\uff0c\u5176\u4ed6\u6570\u636e\u6e90\u53ef\u4ee5\u901a\u8fc7 DataX \u5f80 TuGraph \u91cc\u9762\u5199\u6570\u636e\u3002\nDataX \u4ecb\u7ecd\u53c2\u8003 ",(0,r.jsx)(e.a,{href:"https://github.com/alibaba/DataX",children:"https://github.com/alibaba/DataX"}),"\n\u652f\u6301\u7684\u529f\u80fd\u5305\u62ec\uff1a"]}),"\n",(0,r.jsxs)(e.ul,{children:["\n",(0,r.jsx)(e.li,{children:"\u4ece MySQL\u3001SQL Server\u3001Oracle\u3001PostgreSQL\u3001HDFS\u3001Hive\u3001HBase\u3001OTS\u3001ODPS\u3001Kafka \u7b49\u5404\u79cd\u5f02\u6784\u6570\u636e\u6e90\u5bfc\u5165 TuGraph"}),"\n",(0,r.jsx)(e.li,{children:"\u5c06 TuGraph \u5bfc\u5165\u76f8\u5e94\u7684\u76ee\u6807\u6e90 \uff08\u5f85\u5f00\u53d1\uff09"}),"\n"]}),"\n",(0,r.jsx)(e.p,{children:(0,r.jsx)(e.img,{alt:"\u5bfc\u5165\u5bfc\u51fa",src:a(66467).A+"",width:"1288",height:"404"})}),"\n",(0,r.jsx)(e.h2,{id:"2\u7f16\u8bd1\u5b89\u88c5",children:"2.\u7f16\u8bd1\u5b89\u88c5"}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{className:"language-bash",children:"git clone git@code.alipay.com:fma/DataX.git\nmvn -U clean package assembly:assembly -Dmaven.test.skip=true\n"})}),"\n",(0,r.jsx)(e.p,{children:"\u7f16\u8bd1\u51fa\u6765\u7684 DataX \u6587\u4ef6\u5728 target \u76ee\u5f55\u4e0b"}),"\n",(0,r.jsx)(e.h2,{id:"3\u6587\u672c\u6570\u636e\u901a\u8fc7datax\u5bfc\u5165tugraph",children:"3.\u6587\u672c\u6570\u636e\u901a\u8fc7DataX\u5bfc\u5165TuGraph"}),"\n",(0,r.jsxs)(e.p,{children:["\u6211\u4eec\u4ee5 TuGraph \u624b\u518c\u4e2d\u5bfc\u5165\u5de5\u5177 lgraph_import \u7ae0\u8282\u4e3e\u7684\u6570\u636e\u4e3a\u4f8b\u5b50\uff0c\u6709\u4e09\u4e2a csv \u6570\u636e\u6587\u4ef6\uff0c\u5982\u4e0b\uff1a\n",(0,r.jsx)(e.code,{children:"actors.csv"})]}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{children:"nm015950,Stephen Chow\nnm0628806,Man-Tat Ng\nnm0156444,Cecilia Cheung\nnm2514879,Yuqi Zhang\n"})}),"\n",(0,r.jsx)(e.p,{children:(0,r.jsx)(e.code,{children:"movies.csv"})}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{children:"tt0188766,King of Comedy,1999,7.3\ntt0286112,Shaolin Soccer,2001,7.3\ntt4701660,The Mermaid,2016,6.3\n"})}),"\n",(0,r.jsx)(e.p,{children:(0,r.jsx)(e.code,{children:"roles.csv"})}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{children:"nm015950,Tianchou Yin,tt0188766\nnm015950,Steel Leg,tt0286112\nnm0628806,,tt0188766\nnm0628806,coach,tt0286112\nnm0156444,PiaoPiao Liu,tt0188766\nnm2514879,Ruolan Li,tt4701660\n"})}),"\n",(0,r.jsxs)(e.p,{children:["\u7136\u540e\u5efa\u4e09\u4e2a DataX \u7684 job \u914d\u7f6e\u6587\u4ef6\uff1a\n",(0,r.jsx)(e.code,{children:"job_actors.json"})]}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{className:"language-json",children:'{\n  "job": {\n    "setting": {\n      "speed": {\n        "channel": 1\n      }\n    },\n    "content": [\n      {\n        "reader": {\n          "name": "txtfilereader",\n          "parameter": {\n            "path": ["actors.csv"],\n            "encoding": "UTF-8",\n            "column": [\n              {\n                "index": 0,\n                "type": "string"\n              },\n              {\n                "index": 1,\n                "type": "string"\n              }\n            ],\n            "fieldDelimiter": ","\n          }\n        },\n        "writer": {\n          "name": "tugraphwriter",\n          "parameter": {\n            "host": "127.0.0.1",\n            "port": 7071,\n            "username": "admin",\n            "password": "73@TuGraph",\n            "graphName": "default",\n            "schema": [\n              {\n                "label": "actor",\n                "type": "VERTEX",\n                "properties": [\n                  { "name": "aid", "type": "STRING" },\n                  { "name": "name", "type": "STRING" }\n                ],\n                "primary": "aid"\n              }\n            ],\n            "files": [\n              {\n                "label": "actor",\n                "format": "JSON",\n                "columns": ["aid", "name"]\n              }\n            ]\n          }\n        }\n      }\n    ]\n  }\n}\n'})}),"\n",(0,r.jsx)(e.p,{children:(0,r.jsx)(e.code,{children:"job_movies.json"})}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{className:"language-json",children:'{\n  "job": {\n    "setting": {\n      "speed": {\n        "channel": 1\n      }\n    },\n    "content": [\n      {\n        "reader": {\n          "name": "txtfilereader",\n          "parameter": {\n            "path": ["movies.csv"],\n            "encoding": "UTF-8",\n            "column": [\n              {\n                "index": 0,\n                "type": "string"\n              },\n              {\n                "index": 1,\n                "type": "string"\n              },\n              {\n                "index": 2,\n                "type": "string"\n              },\n              {\n                "index": 3,\n                "type": "string"\n              }\n            ],\n            "fieldDelimiter": ","\n          }\n        },\n        "writer": {\n          "name": "tugraphwriter",\n          "parameter": {\n            "host": "127.0.0.1",\n            "port": 7071,\n            "username": "admin",\n            "password": "73@TuGraph",\n            "graphName": "default",\n            "schema": [\n              {\n                "label": "movie",\n                "type": "VERTEX",\n                "properties": [\n                  { "name": "mid", "type": "STRING" },\n                  { "name": "name", "type": "STRING" },\n                  { "name": "year", "type": "STRING" },\n                  { "name": "rate", "type": "FLOAT", "optional": true }\n                ],\n                "primary": "mid"\n              }\n            ],\n            "files": [\n              {\n                "label": "movie",\n                "format": "JSON",\n                "columns": ["mid", "name", "year", "rate"]\n              }\n            ]\n          }\n        }\n      }\n    ]\n  }\n}\n'})}),"\n",(0,r.jsx)(e.p,{children:(0,r.jsx)(e.code,{children:"job_roles.json"})}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{className:"language-json",children:'{\n  "job": {\n    "setting": {\n      "speed": {\n        "channel": 1\n      }\n    },\n    "content": [\n      {\n        "reader": {\n          "name": "txtfilereader",\n          "parameter": {\n            "path": ["roles.csv"],\n            "encoding": "UTF-8",\n            "column": [\n              {\n                "index": 0,\n                "type": "string"\n              },\n              {\n                "index": 1,\n                "type": "string"\n              },\n              {\n                "index": 2,\n                "type": "string"\n              }\n            ],\n            "fieldDelimiter": ","\n          }\n        },\n        "writer": {\n          "name": "tugraphwriter",\n          "parameter": {\n            "host": "127.0.0.1",\n            "port": 7071,\n            "username": "admin",\n            "password": "73@TuGraph",\n            "graphName": "default",\n            "schema": [\n              {\n                "label": "play_in",\n                "type": "EDGE",\n                "properties": [{ "name": "role", "type": "STRING" }]\n              }\n            ],\n            "files": [\n              {\n                "label": "play_in",\n                "format": "JSON",\n                "SRC_ID": "actor",\n                "DST_ID": "movie",\n                "columns": ["SRC_ID", "role", "DST_ID"]\n              }\n            ]\n          }\n        }\n      }\n    ]\n  }\n}\n'})}),"\n",(0,r.jsxs)(e.p,{children:[(0,r.jsx)(e.code,{children:"./lgraph_server -c lgraph_standalone.json -d 'run'"})," \u542f\u52a8 TuGraph \u540e\u4f9d\u6b21\u6267\u884c\u5982\u4e0b\u4e09\u4e2a\u547d\u4ee4\uff1a"]}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{children:"python3 datax/bin/datax.py  job_actors.json\n"})}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{children:"python3 datax/bin/datax.py  job_movies.json\n"})}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{children:"python3 datax/bin/datax.py  job_roles.json\n"})}),"\n",(0,r.jsx)(e.h2,{id:"4mysql\u6570\u636e\u901a\u8fc7datax\u5bfc\u5165tugraph",children:"4.MySQL\u6570\u636e\u901a\u8fc7DataX\u5bfc\u5165TuGraph"}),"\n",(0,r.jsxs)(e.p,{children:["\u6211\u4eec\u5728 ",(0,r.jsx)(e.code,{children:"test"})," database \u4e0b\u5efa\u7acb\u5982\u4e0b\u7535\u5f71 ",(0,r.jsx)(e.code,{children:"movies"})," \u8868"]}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{className:"language-sql",children:"CREATE TABLE `movies` (\n  `mid`  varchar(200) NOT NULL,\n  `name` varchar(100) NOT NULL,\n  `year` int(11) NOT NULL,\n  `rate` float(5,2) unsigned NOT NULL,\n  PRIMARY KEY (`mid`)\n);\n"})}),"\n",(0,r.jsx)(e.p,{children:"\u5f80\u8868\u4e2d\u63d2\u5165\u51e0\u6761\u6570\u636e"}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{className:"language-sql",children:"insert into\ntest.movies (mid, name, year, rate)\nvalues\n('tt0188766', 'King of Comedy', 1999, 7.3),\n('tt0286112', 'Shaolin Soccer', 2001, 7.3),\n('tt4701660', 'The Mermaid',   2016,  6.3);\n"})}),"\n",(0,r.jsx)(e.p,{children:"\u5efa\u7acb\u4e00\u4e2a DataX \u7684 job \u914d\u7f6e\u6587\u4ef6"}),"\n",(0,r.jsx)(e.p,{children:(0,r.jsx)(e.code,{children:"job_mysql_to_tugraph.json"})}),"\n",(0,r.jsx)(e.p,{children:(0,r.jsx)(e.strong,{children:"\u914d\u7f6e\u5b57\u6bb5\u65b9\u5f0f"})}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{className:"language-json",children:'{\n  "job": {\n    "setting": {\n      "speed": {\n        "channel": 1\n      }\n    },\n    "content": [\n      {\n        "reader": {\n          "name": "mysqlreader",\n          "parameter": {\n            "username": "root",\n            "password": "root",\n            "column": ["mid", "name", "year", "rate"],\n            "splitPk": "mid",\n            "connection": [\n              {\n                "table": ["movies"],\n                "jdbcUrl": ["jdbc:mysql://127.0.0.1:3306/test?useSSL=false"]\n              }\n            ]\n          }\n        },\n        "writer": {\n          "name": "tugraphwriter",\n          "parameter": {\n            "host": "127.0.0.1",\n            "port": 7071,\n            "username": "admin",\n            "password": "73@TuGraph",\n            "graphName": "default",\n            "schema": [\n              {\n                "label": "movie",\n                "type": "VERTEX",\n                "properties": [\n                  { "name": "mid", "type": "STRING" },\n                  { "name": "name", "type": "STRING" },\n                  { "name": "year", "type": "STRING" },\n                  { "name": "rate", "type": "FLOAT", "optional": true }\n                ],\n                "primary": "mid"\n              }\n            ],\n            "files": [\n              {\n                "label": "movie",\n                "format": "JSON",\n                "columns": ["mid", "name", "year", "rate"]\n              }\n            ]\n          }\n        }\n      }\n    ]\n  }\n}\n'})}),"\n",(0,r.jsx)(e.p,{children:(0,r.jsx)(e.strong,{children:"\u5199\u7b80\u5355 sql \u65b9\u5f0f"})}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{className:"language-json",children:'{\n  "job": {\n    "setting": {\n      "speed": {\n        "channel": 1\n      }\n    },\n    "content": [\n      {\n        "reader": {\n          "name": "mysqlreader",\n          "parameter": {\n            "username": "root",\n            "password": "root",\n            "connection": [\n              {\n                "querySql": [\n                  "select mid, name, year, rate from test.movies where year > 2000;"\n                ],\n                "jdbcUrl": ["jdbc:mysql://127.0.0.1:3306/test?useSSL=false"]\n              }\n            ]\n          }\n        },\n        "writer": {\n          "name": "tugraphwriter",\n          "parameter": {\n            "host": "127.0.0.1",\n            "port": 7071,\n            "username": "admin",\n            "password": "73@TuGraph",\n            "graphName": "default",\n            "schema": [\n              {\n                "label": "movie",\n                "type": "VERTEX",\n                "properties": [\n                  { "name": "mid", "type": "STRING" },\n                  { "name": "name", "type": "STRING" },\n                  { "name": "year", "type": "STRING" },\n                  { "name": "rate", "type": "FLOAT", "optional": true }\n                ],\n                "primary": "mid"\n              }\n            ],\n            "files": [\n              {\n                "label": "movie",\n                "format": "JSON",\n                "columns": ["mid", "name", "year", "rate"]\n              }\n            ]\n          }\n        }\n      }\n    ]\n  }\n}\n'})}),"\n",(0,r.jsxs)(e.p,{children:[(0,r.jsx)(e.code,{children:"./lgraph_server -c lgraph_standalone.json -d 'run'"})," \u542f\u52a8 TuGraph \u540e\u6267\u884c\u5982\u4e0b\u547d\u4ee4\uff1a"]}),"\n",(0,r.jsx)(e.pre,{children:(0,r.jsx)(e.code,{className:"language-shell",children:"python3 datax/bin/datax.py  job_mysql_to_tugraph.json\n"})})]})}function p(n={}){const{wrapper:e}={...(0,t.R)(),...n.components};return e?(0,r.jsx)(e,{...n,children:(0,r.jsx)(c,{...n})}):c(n)}}}]);