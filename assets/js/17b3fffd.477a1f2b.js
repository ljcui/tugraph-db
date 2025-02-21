"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[77120],{28453:(e,n,r)=>{r.d(n,{R:()=>c,x:()=>l});var i=r(96540);const o={},s=i.createContext(o);function c(e){const n=i.useContext(s);return i.useMemo((function(){return"function"==typeof e?e(n):{...n,...e}}),[n,e])}function l(e){let n;return n=e.disableParentContext?"function"==typeof e.components?e.components(o):e.components||o:c(e.components),i.createElement(s.Provider,{value:n},e.children)}},42440:(e,n,r)=>{r.r(n),r.d(n,{assets:()=>t,contentTitle:()=>c,default:()=>u,frontMatter:()=>s,metadata:()=>l,toc:()=>d});var i=r(74848),o=r(28453);const s={},c="Compile",l={id:"developer-manual/running/compile",title:"Compile",description:"This document mainly describes how to compile TuGraph from source code.",source:"@site/versions/version-3.6.0/en-US/source/5.developer-manual/2.running/1.compile.md",sourceDirName:"5.developer-manual/2.running",slug:"/developer-manual/running/compile",permalink:"/tugraph-db/en/3.6.0/developer-manual/running/compile",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:1,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"Cloud Deployment",permalink:"/tugraph-db/en/3.6.0/developer-manual/installation/cloud-deployment"},next:{title:"Tugraph Running",permalink:"/tugraph-db/en/3.6.0/developer-manual/running/tugraph-running"}},t={},d=[{value:"1.Prerequisites",id:"1prerequisites",level:2},{value:"2.compile",id:"2compile",level:2}];function a(e){const n={a:"a",blockquote:"blockquote",code:"code",h1:"h1",h2:"h2",header:"header",li:"li",ol:"ol",p:"p",pre:"pre",...(0,o.R)(),...e.components};return(0,i.jsxs)(i.Fragment,{children:[(0,i.jsx)(n.header,{children:(0,i.jsx)(n.h1,{id:"compile",children:"Compile"})}),"\n",(0,i.jsxs)(n.blockquote,{children:["\n",(0,i.jsx)(n.p,{children:"This document mainly describes how to compile TuGraph from source code."}),"\n"]}),"\n",(0,i.jsx)(n.h2,{id:"1prerequisites",children:"1.Prerequisites"}),"\n",(0,i.jsxs)(n.p,{children:["It is recommended to build TuGraph on a Linux system. Meanwhile, Docker is a good choice. If you want to set up a new environment, please refer to ",(0,i.jsx)(n.a,{href:"/tugraph-db/en/3.6.0/developer-manual/installation/docker-deployment",children:"Dockerfile"}),"\u3002"]}),"\n",(0,i.jsx)(n.h2,{id:"2compile",children:"2.compile"}),"\n",(0,i.jsx)(n.p,{children:"Here are steps to compile TuGraph:"}),"\n",(0,i.jsxs)(n.ol,{children:["\n",(0,i.jsxs)(n.li,{children:[(0,i.jsx)(n.code,{children:"deps/build_deps.sh"})," or to skip building web interface ",(0,i.jsx)(n.code,{children:"SKIP_WEB=1 deps/build_deps.sh"})]}),"\n",(0,i.jsxs)(n.li,{children:[(0,i.jsx)(n.code,{children:"cmake .. -DOURSYSTEM=centos"})," or ",(0,i.jsx)(n.code,{children:"cmake .. -DOURSYSTEM=ubuntu"})]}),"\n",(0,i.jsxs)(n.li,{children:["If support shell lgraph_cypher, use ",(0,i.jsx)(n.code,{children:"-DENABLE_PREDOWNLOAD_DEPENDS_PACKAGE=1"})]}),"\n",(0,i.jsx)(n.li,{children:(0,i.jsx)(n.code,{children:"make"})}),"\n",(0,i.jsxs)(n.li,{children:[(0,i.jsx)(n.code,{children:"make package"})," or ",(0,i.jsx)(n.code,{children:"cpack --config CPackConfig.cmake"})]}),"\n"]}),"\n",(0,i.jsxs)(n.p,{children:["Example:\n",(0,i.jsx)(n.code,{children:"tugraph/tugraph-compile-centos7"}),"Docker environment"]}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-bash",children:"$ git clone --recursive https://github.com/TuGraph-family/tugraph-db.git\n$ cd tugraph-db\n$ deps/build_deps.sh\n$ mkdir build && cd build\n$ cmake .. -DOURSYSTEM=centos -DENABLE_PREDOWNLOAD_DEPENDS_PACKAGE=1\n$ make\n$ make package\n"})})]})}function u(e={}){const{wrapper:n}={...(0,o.R)(),...e.components};return n?(0,i.jsx)(n,{...e,children:(0,i.jsx)(a,{...e})}):a(e)}}}]);