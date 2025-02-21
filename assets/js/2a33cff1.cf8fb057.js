"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[75570],{28453:(e,n,i)=>{i.d(n,{R:()=>s,x:()=>c});var r=i(96540);const o={},t=r.createContext(o);function s(e){const n=r.useContext(t);return r.useMemo((function(){return"function"==typeof e?e(n):{...n,...e}}),[n,e])}function c(e){let n;return n=e.disableParentContext?"function"==typeof e.components?e.components(o):e.components||o:s(e.components),r.createElement(t.Provider,{value:n},e.children)}},62982:(e,n,i)=>{i.r(n),i.d(n,{assets:()=>l,contentTitle:()=>s,default:()=>u,frontMatter:()=>t,metadata:()=>c,toc:()=>a});var r=i(74848),o=i(28453);const t={},s="Compile",c={id:"installation&running/compile",title:"Compile",description:"This document mainly describes how to compile TuGraph from source code.",source:"@site/versions/version-4.3.0/en-US/source/5.installation&running/6.compile.md",sourceDirName:"5.installation&running",slug:"/installation&running/compile",permalink:"/tugraph-db/en/4.3.0/installation&running/compile",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:6,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"Cloud Deployment",permalink:"/tugraph-db/en/4.3.0/installation&running/cloud-deployment"},next:{title:"Tugraph Running",permalink:"/tugraph-db/en/4.3.0/installation&running/tugraph-running"}},l={},a=[{value:"1.Prerequisites",id:"1prerequisites",level:2},{value:"2.compile",id:"2compile",level:2}];function d(e){const n={a:"a",blockquote:"blockquote",code:"code",h1:"h1",h2:"h2",header:"header",li:"li",ol:"ol",p:"p",pre:"pre",...(0,o.R)(),...e.components};return(0,r.jsxs)(r.Fragment,{children:[(0,r.jsx)(n.header,{children:(0,r.jsx)(n.h1,{id:"compile",children:"Compile"})}),"\n",(0,r.jsxs)(n.blockquote,{children:["\n",(0,r.jsx)(n.p,{children:"This document mainly describes how to compile TuGraph from source code."}),"\n"]}),"\n",(0,r.jsx)(n.h2,{id:"1prerequisites",children:"1.Prerequisites"}),"\n",(0,r.jsxs)(n.p,{children:["It is recommended to build TuGraph on a Linux system. Meanwhile, Docker is a good choice. If you want to set up a new environment, please refer to ",(0,r.jsx)(n.a,{href:"/tugraph-db/en/4.3.0/installation&running/docker-deployment",children:"Dockerfile"}),"\u3002"]}),"\n",(0,r.jsx)(n.h2,{id:"2compile",children:"2.compile"}),"\n",(0,r.jsx)(n.p,{children:"Here are steps to compile TuGraph:"}),"\n",(0,r.jsxs)(n.ol,{children:["\n",(0,r.jsxs)(n.li,{children:["run ",(0,r.jsx)(n.code,{children:"deps/build_deps.sh"})," to build tugraph-web if you need. Skip this step otherwise."]}),"\n",(0,r.jsxs)(n.li,{children:[(0,r.jsx)(n.code,{children:"cmake .. -DOURSYSTEM=centos"})," or ",(0,r.jsx)(n.code,{children:"cmake .. -DOURSYSTEM=ubuntu"})]}),"\n",(0,r.jsxs)(n.li,{children:["If support shell lgraph_cypher, use ",(0,r.jsx)(n.code,{children:"-DENABLE_PREDOWNLOAD_DEPENDS_PACKAGE=1"})]}),"\n",(0,r.jsxs)(n.li,{children:["If compiling on an arm machine (such as a Mac with M1 chip), add ",(0,r.jsx)(n.code,{children:"-DENABLE_BUILD_ON_AARCH64=ON"}),"\n5",(0,r.jsx)(n.code,{children:"make"}),"\n6",(0,r.jsx)(n.code,{children:"make package"})," or ",(0,r.jsx)(n.code,{children:"cpack --config CPackConfig.cmake"})]}),"\n"]}),"\n",(0,r.jsxs)(n.p,{children:["Example:\n",(0,r.jsx)(n.code,{children:"tugraph/tugraph-compile-centos7"}),"Docker environment"]}),"\n",(0,r.jsx)(n.pre,{children:(0,r.jsx)(n.code,{className:"language-bash",children:"$ git clone --recursive https://github.com/TuGraph-family/tugraph-db.git\n$ cd tugraph-db\n$ deps/build_deps.sh\n$ mkdir build && cd build\n$ cmake .. -DOURSYSTEM=centos -DENABLE_PREDOWNLOAD_DEPENDS_PACKAGE=1\n$ make\n$ make package\n"})})]})}function u(e={}){const{wrapper:n}={...(0,o.R)(),...e.components};return n?(0,r.jsx)(n,{...e,children:(0,r.jsx)(d,{...e})}):d(e)}}}]);