"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[29983],{4399:(n,e,s)=>{s.d(e,{A:()=>d});const d=s.p+"assets/images/cloud-deployment-6-66d573e42075d2367c35e77b3330e2ac.png"},20413:(n,e,s)=>{s.d(e,{A:()=>d});const d=s.p+"assets/images/cloud-deployment-4-0372e82170ac8bffe6d2d02b03b0a9e2.png"},28453:(n,e,s)=>{s.d(e,{R:()=>r,x:()=>t});var d=s(96540);const l={},i=d.createContext(l);function r(n){const e=d.useContext(i);return d.useMemo((function(){return"function"==typeof n?n(e):{...e,...n}}),[e,n])}function t(n){let e;return e=n.disableParentContext?"function"==typeof n.components?n.components(l):n.components||l:r(n.components),d.createElement(i.Provider,{value:e},n.children)}},29592:(n,e,s)=>{s.d(e,{A:()=>d});const d=s.p+"assets/images/cloud-deployment-9-8778b1296f1ca19397384c068bab67ea.png"},35376:(n,e,s)=>{s.d(e,{A:()=>d});const d=s.p+"assets/images/cloud-deployment-1-6de73b2cfb5fe70e92e5e3435ad8574f.png"},39206:(n,e,s)=>{s.d(e,{A:()=>d});const d=s.p+"assets/images/cloud-deployment-7-a23a6b3eff495502a4c3dcb7e92e19ca.png"},49195:(n,e,s)=>{s.d(e,{A:()=>d});const d=s.p+"assets/images/cloud-deployment-2-2424fad73dbcd8267de47772bd60d29f.png"},50721:(n,e,s)=>{s.d(e,{A:()=>d});const d=s.p+"assets/images/cloud-deployment-8-3141da1eed5c4e147ecdef322cf1c58d.png"},53833:(n,e,s)=>{s.r(e),s.d(e,{assets:()=>c,contentTitle:()=>r,default:()=>x,frontMatter:()=>i,metadata:()=>t,toc:()=>h});var d=s(74848),l=s(28453);const i={},r="\u4e91\u90e8\u7f72",t={id:"developer-manual/installation/cloud-deployment",title:"\u4e91\u90e8\u7f72",description:"\u6b64\u6587\u6863\u4e3b\u8981\u4ecb\u7ecd TuGraph \u7684\u4e91\u90e8\u7f72\uff0c\u4e5f\u53ef\u53c2\u89c1\u963f\u91cc\u4e91\u8ba1\u7b97\u5de2\u90e8\u7f72\u6587\u6863\u3002",source:"@site/versions/version-4.0.1/zh-CN/source/5.developer-manual/1.installation/5.cloud-deployment.md",sourceDirName:"5.developer-manual/1.installation",slug:"/developer-manual/installation/cloud-deployment",permalink:"/tugraph-db/en-US/zh/4.0.1/developer-manual/installation/cloud-deployment",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:5,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"\u672c\u5730\u5305\u90e8\u7f72",permalink:"/tugraph-db/en-US/zh/4.0.1/developer-manual/installation/local-package-deployment"},next:{title:"\u4ece\u6e90\u7801\u7f16\u8bd1",permalink:"/tugraph-db/en-US/zh/4.0.1/developer-manual/running/compile"}},c={},h=[{value:"1.\u7b80\u4ecb",id:"1\u7b80\u4ecb",level:2},{value:"2.\u5b9e\u4f8b\u8bf4\u660e",id:"2\u5b9e\u4f8b\u8bf4\u660e",level:2},{value:"3.\u90e8\u7f72\u6d41\u7a0b",id:"3\u90e8\u7f72\u6d41\u7a0b",level:2},{value:"3.1.\u51c6\u5907\u5de5\u4f5c",id:"31\u51c6\u5907\u5de5\u4f5c",level:3},{value:"3.2.\u90e8\u7f72\u5165\u53e3",id:"32\u90e8\u7f72\u5165\u53e3",level:3},{value:"3.3.\u7533\u8bf7\u8bd5\u7528",id:"33\u7533\u8bf7\u8bd5\u7528",level:3},{value:"3.4.\u521b\u5efaTuGraph\u670d\u52a1",id:"34\u521b\u5efatugraph\u670d\u52a1",level:3},{value:"3.4.1.\u53c2\u6570\u5217\u8868",id:"341\u53c2\u6570\u5217\u8868",level:4},{value:"3.4.2.\u5177\u4f53\u6b65\u9aa4",id:"342\u5177\u4f53\u6b65\u9aa4",level:4},{value:"3.5.\u542f\u52a8TuGraph\u670d\u52a1",id:"35\u542f\u52a8tugraph\u670d\u52a1",level:3},{value:"4.\u5e38\u89c1FAQ",id:"4\u5e38\u89c1faq",level:2},{value:"\u95ee\u9898\u4e00\uff1a\u90e8\u7f72\u533a\u57df\u65e0\u53ef\u7528\u8d44\u6e90",id:"\u95ee\u9898\u4e00\u90e8\u7f72\u533a\u57df\u65e0\u53ef\u7528\u8d44\u6e90",level:3},{value:"\u95ee\u9898\u4e8c: \u542f\u52a8\u540eweb\u8bbf\u95ee\u4e0d\u901a",id:"\u95ee\u9898\u4e8c-\u542f\u52a8\u540eweb\u8bbf\u95ee\u4e0d\u901a",level:3},{value:"\u95ee\u9898\u4e09: \u767b\u5f55\u65f6\u7684\u7528\u6237\u540d\u5bc6\u7801\u4e0d\u6b63\u786e",id:"\u95ee\u9898\u4e09-\u767b\u5f55\u65f6\u7684\u7528\u6237\u540d\u5bc6\u7801\u4e0d\u6b63\u786e",level:3}];function a(n){const e={a:"a",blockquote:"blockquote",del:"del",h1:"h1",h2:"h2",h3:"h3",h4:"h4",header:"header",img:"img",li:"li",p:"p",strong:"strong",table:"table",tbody:"tbody",td:"td",th:"th",thead:"thead",tr:"tr",ul:"ul",...(0,l.R)(),...n.components};return(0,d.jsxs)(d.Fragment,{children:[(0,d.jsx)(e.header,{children:(0,d.jsx)(e.h1,{id:"\u4e91\u90e8\u7f72",children:"\u4e91\u90e8\u7f72"})}),"\n",(0,d.jsxs)(e.blockquote,{children:["\n",(0,d.jsxs)(e.p,{children:["\u6b64\u6587\u6863\u4e3b\u8981\u4ecb\u7ecd TuGraph \u7684\u4e91\u90e8\u7f72\uff0c\u4e5f\u53ef\u53c2\u89c1",(0,d.jsx)(e.a,{href:"https://aliyun-computenest.github.io/quickstart-tugraph/",children:"\u963f\u91cc\u4e91\u8ba1\u7b97\u5de2\u90e8\u7f72\u6587\u6863"}),"\u3002"]}),"\n"]}),"\n",(0,d.jsx)(e.h2,{id:"1\u7b80\u4ecb",children:"1.\u7b80\u4ecb"}),"\n",(0,d.jsx)(e.p,{children:"TuGraph\uff08tugraph.antgroup.com\uff09\u662f\u8682\u8681\u96c6\u56e2\u7814\u53d1\u7684\u9ad8\u6027\u80fd\u56fe\u6570\u636e\u5e93\uff08Graph Database\uff09\u3002TuGraph\u5728\u8ba1\u7b97\u5de2\u4e0a\u63d0\u4f9b\u4e86\u793e\u533a\u7248\u670d\u52a1\uff0c\u60a8\u65e0\u9700\u81ea\u884c\u8d2d\u7f6e\u4e91\u4e3b\u673a\uff0c\u5373\u53ef\u5728\u8ba1\u7b97\u5de2\u4e0a\u5feb\u901f\u90e8\u7f72TuGraph\u670d\u52a1\u3001\u5b9e\u73b0\u8fd0\u7ef4\u76d1\u63a7\uff0c\u4ece\u800c\u642d\u5efa\u60a8\u81ea\u5df1\u7684\u56fe\u5e94\u7528\u3002\u672c\u6587\u5411\u60a8\u4ecb\u7ecd\u5982\u4f55\u5f00\u901a\u8ba1\u7b97\u5de2\u4e0a\u7684TuGraph\u793e\u533a\u7248\u670d\u52a1\uff0c\u4ee5\u53ca\u90e8\u7f72\u6d41\u7a0b\u548c\u4f7f\u7528\u8bf4\u660e\u3002"}),"\n",(0,d.jsx)(e.h2,{id:"2\u5b9e\u4f8b\u8bf4\u660e",children:"2.\u5b9e\u4f8b\u8bf4\u660e"}),"\n",(0,d.jsx)(e.p,{children:"TuGraph\u90e8\u7f72\u7684\u4e3a\u793e\u533a\u5f00\u6e90\u7248\u672c\uff0c\u6e90\u7801\u53c2\u8003Github Repo\uff0c\u76ee\u524d\u53ef\u4ee5\u9009\u62e9\u7684\u5b9e\u4f8b\u89c4\u683c\u5982\u4e0b\uff1a"}),"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",(0,d.jsxs)(e.table,{children:[(0,d.jsx)(e.thead,{children:(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.th,{children:"\u89c4\u683c\u65cf"}),(0,d.jsx)(e.th,{children:"vCPU\u4e0e\u5185\u5b58"}),(0,d.jsx)(e.th,{children:"\u7cfb\u7edf\u76d8"}),(0,d.jsx)(e.th,{children:"\u516c\u7f51\u5e26\u5bbd"})]})}),(0,d.jsxs)(e.tbody,{children:[(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"ecs.r7a.xlarge"}),(0,d.jsx)(e.td,{children:"AMD \u5185\u5b58\u578b r7a\uff0c4vCPU 32GiB"}),(0,d.jsx)(e.td,{children:"ESSD\u4e91\u76d8 200GiB PL0"}),(0,d.jsx)(e.td,{children:"\u56fa\u5b9a\u5e26\u5bbd1Mbps"})]}),(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"ecs.r6.xlarge"}),(0,d.jsx)(e.td,{children:"\u5185\u5b58\u578br6\uff0c4vCPU 32GiB"}),(0,d.jsx)(e.td,{children:"ESSD\u4e91\u76d8 200GiB PL0"}),(0,d.jsx)(e.td,{children:"\u56fa\u5b9a\u5e26\u5bbd1Mbps"})]})]})]}),"\n",(0,d.jsxs)(e.p,{children:["\u9884\u4f30\u8d39\u7528\u5728\u521b\u5efa\u5b9e\u4f8b\u65f6\u53ef\u5b9e\u65f6\u770b\u5230\uff08\u76ee\u524d\u4e3a\u514d\u8d39\uff09\u3002 \u5982\u9700\u66f4\u591a\u89c4\u683c\u3001\u5176\u4ed6\u670d\u52a1\uff08\u5982\u96c6\u7fa4\u9ad8\u53ef\u7528\u6027\u8981\u6c42\u3001\u4f01\u4e1a\u7ea7\u652f\u6301\u670d\u52a1\u7b49\uff09\uff0c\u8bf7\u8054\u7cfb\u6211\u4eec ",(0,d.jsx)(e.a,{href:"mailto:tugraph@service.alipay.com",children:"tugraph@service.alipay.com"}),"\u3002"]}),"\n",(0,d.jsx)(e.h2,{id:"3\u90e8\u7f72\u6d41\u7a0b",children:"3.\u90e8\u7f72\u6d41\u7a0b"}),"\n",(0,d.jsx)(e.h3,{id:"31\u51c6\u5907\u5de5\u4f5c",children:"3.1.\u51c6\u5907\u5de5\u4f5c"}),"\n",(0,d.jsx)(e.p,{children:"\u5728\u6b63\u5f0f\u5f00\u59cb\u4f7f\u7528\u524d\uff0c\u60a8\u9700\u8981\u4e00\u4e2a\u963f\u91cc\u4e91\u8d26\u53f7\uff0c\u5bf9ECS\u3001VPC\u7b49\u8d44\u6e90\u8fdb\u884c\u8bbf\u95ee\u548c\u521b\u5efa\u64cd\u4f5c\u3002"}),"\n",(0,d.jsxs)(e.ul,{children:["\n",(0,d.jsx)(e.li,{children:"\u82e5\u60a8\u4f7f\u7528\u4e2a\u4eba\u8d26\u53f7\uff0c\u53ef\u4ee5\u76f4\u63a5\u521b\u5efa\u670d\u52a1\u5b9e\u4f8b"}),"\n",(0,d.jsxs)(e.li,{children:["\u82e5\u60a8\u4f7f\u7528RAM\u7528\u6237\u521b\u5efa\u670d\u52a1\u5b9e\u4f8b\uff0c\u4e14\u662f\u7b2c\u4e00\u6b21\u4f7f\u7528\u963f\u91cc\u4e91\u8ba1\u7b97\u5de2\n",(0,d.jsxs)(e.ul,{children:["\n",(0,d.jsxs)(e.li,{children:["\u9700\u8981\u5728\u521b\u5efa\u670d\u52a1\u5b9e\u4f8b\u524d\uff0c\u5bf9\u4f7f\u7528\u7684RAM\u7528\u6237\u7684\u8d26\u53f7\u6dfb\u52a0\u76f8\u5e94\u8d44\u6e90\u7684\u6743\u9650\u3002\u6dfb\u52a0RAM\u6743\u9650\u7684\u8be6\u7ec6\u64cd\u4f5c\uff0c\u8bf7\u53c2\u89c1\u4e3a ",(0,d.jsx)(e.strong,{children:"RAM\u7528\u6237\u6388\u6743"}),"\u3002\u6240\u9700\u6743\u9650\u5982\u4e0b\u8868\u6240\u793a\u3002"]}),"\n",(0,d.jsxs)(e.li,{children:["\u4e14\u9700\u8981\u6388\u6743\u521b\u5efa\u5173\u8054\u89d2\u8272\uff0c\u53c2\u8003\u4e0b\u56fe\uff0c\u9009\u4e2d ",(0,d.jsx)(e.strong,{children:"\u540c\u610f\u6388\u6743\u5e76\u521b\u5efa\u5173\u8054\u89d2\u8272"})]}),"\n"]}),"\n"]}),"\n"]}),"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",(0,d.jsxs)(e.table,{children:[(0,d.jsx)(e.thead,{children:(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.th,{children:"\u6743\u9650\u7b56\u7565\u540d\u79f0"}),(0,d.jsx)(e.th,{children:"\u5907\u6ce8"})]})}),(0,d.jsxs)(e.tbody,{children:[(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"AliyunECSFullAccess"}),(0,d.jsx)(e.td,{children:"\u7ba1\u7406\u4e91\u670d\u52a1\u5668\u670d\u52a1\uff08ECS\uff09\u7684\u6743\u9650"})]}),(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"AliyunVPCFullAccess"}),(0,d.jsx)(e.td,{children:"\u7ba1\u7406\u4e13\u6709\u7f51\u7edc\uff08VPC\uff09\u7684\u6743\u9650"})]}),(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"AliyunROSFullAccess"}),(0,d.jsx)(e.td,{children:"\u7ba1\u7406\u8d44\u6e90\u7f16\u6392\u670d\u52a1\uff08ROS\uff09\u7684\u6743\u9650"})]}),(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"AliyunComputeNestUserFullAccess"}),(0,d.jsx)(e.td,{children:"\u7ba1\u7406\u8ba1\u7b97\u5de2\u670d\u52a1\uff08ComputeNest\uff09\u7684\u7528\u6237\u4fa7\u6743\u9650"})]}),(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"AliyunCloudMonitorFullAccess"}),(0,d.jsx)(e.td,{children:"\u7ba1\u7406\u4e91\u76d1\u63a7\uff08CloudMonitor\uff09\u7684\u6743\u9650"})]})]})]}),"\n",(0,d.jsx)(e.p,{children:(0,d.jsx)(e.img,{alt:"\u540c\u610f\u6388\u6743\u5e76\u521b\u5efa\u5173\u8054\u89d2\u8272",src:s(35376).A+"",width:"2390",height:"612"})}),"\n",(0,d.jsx)(e.h3,{id:"32\u90e8\u7f72\u5165\u53e3",children:"3.2.\u90e8\u7f72\u5165\u53e3"}),"\n",(0,d.jsx)(e.p,{children:"\u60a8\u53ef\u4ee5\u5728\u963f\u91cc\u4e91\u8ba1\u7b97\u5de2\u81ea\u884c\u641c\u7d22\uff0c\u4e5f\u53ef\u4ee5\u901a\u8fc7\u4e0b\u8ff0\u90e8\u7f72\u94fe\u63a5\u5feb\u901f\u5230\u8fbe\u3002"}),"\n",(0,d.jsx)(e.p,{children:(0,d.jsx)(e.a,{href:"https://computenest.console.aliyun.com/user/cn-hangzhou/serviceInstanceCreate?ServiceId=service-7b50ea3d20e643da95bf&&isTrial=true",children:"\u90e8\u7f72\u94fe\u63a5"})}),"\n",(0,d.jsx)(e.h3,{id:"33\u7533\u8bf7\u8bd5\u7528",children:"3.3.\u7533\u8bf7\u8bd5\u7528"}),"\n",(0,d.jsx)(e.p,{children:"\u5728\u6b63\u5f0f\u8bd5\u7528\u524d\uff0c\u9700\u8981\u7533\u8bf7\u8bd5\u7528\uff0c\u6309\u7167\u63d0\u793a\u586b\u5199\u4fe1\u606f\uff0c\u5728\u5ba1\u6838\u901a\u8fc7\u540e\u5c31\u53ef\u4ee5\u521b\u5efaTuGraph\u670d\u52a1\u3002"}),"\n",(0,d.jsx)(e.p,{children:(0,d.jsx)(e.img,{alt:"\u7533\u8bf7\u8bd5\u7528",src:s(49195).A+"",width:"2071",height:"949"})}),"\n",(0,d.jsx)(e.h3,{id:"34\u521b\u5efatugraph\u670d\u52a1",children:"3.4.\u521b\u5efaTuGraph\u670d\u52a1"}),"\n",(0,d.jsx)(e.h4,{id:"341\u53c2\u6570\u5217\u8868",children:"3.4.1.\u53c2\u6570\u5217\u8868"}),"\n",(0,d.jsx)(e.p,{children:"\u60a8\u5728\u521b\u5efa\u670d\u52a1\u5b9e\u4f8b\u7684\u8fc7\u7a0b\u4e2d\uff0c\u9700\u8981\u914d\u7f6e\u670d\u52a1\u5b9e\u4f8b\u4fe1\u606f\u7684\u53c2\u6570\u5217\u8868\uff0c\u5177\u4f53\u5982\u4e0b\u3002"}),"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",(0,d.jsxs)(e.table,{children:[(0,d.jsx)(e.thead,{children:(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.th,{children:"\u53c2\u6570\u7ec4"}),(0,d.jsx)(e.th,{children:"\u53c2\u6570\u9879"}),(0,d.jsx)(e.th,{children:"\u793a\u4f8b"}),(0,d.jsx)(e.th,{children:"\u8bf4\u660e"})]})}),(0,d.jsxs)(e.tbody,{children:[(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"\u670d\u52a1\u5b9e\u4f8b\u540d\u79f0"}),(0,d.jsx)(e.td,{children:"N/A"}),(0,d.jsx)(e.td,{children:"test"}),(0,d.jsx)(e.td,{children:"\u5b9e\u4f8b\u7684\u540d\u79f0"})]}),(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"\u5730\u57df"}),(0,d.jsx)(e.td,{children:"N/A"}),(0,d.jsx)(e.td,{children:"\u534e\u4e1c1\uff08\u676d\u5dde\uff09"}),(0,d.jsx)(e.td,{children:"\u9009\u4e2d\u670d\u52a1\u5b9e\u4f8b\u7684\u5730\u57df\uff0c\u5efa\u8bae\u5c31\u8fd1\u9009\u4e2d\uff0c\u4ee5\u83b7\u53d6\u66f4\u597d\u7684\u7f51\u7edc\u5ef6\u65f6\u3002"})]}),(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"\u4ed8\u8d39\u7c7b\u578b\u914d\u7f6e"}),(0,d.jsx)(e.td,{children:"\u4ed8\u8d39\u7c7b\u578b"}),(0,d.jsx)(e.td,{children:"\u6309\u91cf\u4ed8\u8d39"}),(0,d.jsx)(e.td,{children:"\u514d\u8d39\u4f7f\u7528\u8bf7\u9009\u7528\u6309\u91cf\u4ed8\u8d39"})]}),(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"\u53ef\u7528\u533a\u914d\u7f6e"}),(0,d.jsx)(e.td,{children:"\u90e8\u7f72\u533a\u57df"}),(0,d.jsx)(e.td,{children:"\u53ef\u7528\u533aI"}),(0,d.jsx)(e.td,{children:"\u5730\u57df\u4e0b\u7684\u4e0d\u540c\u53ef\u7528\u533a\u57df\uff0c\u786e\u4fdd\u5b9e\u4f8b\u975e\u7a7a"})]}),(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"\u9009\u62e9\u5df2\u6709\u57fa\u7840\u8d44\u6e90\u914d\u7f6e"}),(0,d.jsx)(e.td,{children:"VPC ID"}),(0,d.jsx)(e.td,{children:"vpc-xxx"}),(0,d.jsx)(e.td,{children:"\u6309\u5b9e\u9645\u60c5\u51b5\uff0c\u9009\u62e9\u4e13\u6709\u7f51\u7edc\u7684ID\u3002"})]}),(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"\u9009\u62e9\u5df2\u6709\u57fa\u7840\u8d44\u6e90\u914d\u7f6e"}),(0,d.jsx)(e.td,{children:"\u4ea4\u6362\u673aID"}),(0,d.jsx)(e.td,{children:"vsw-xxx"}),(0,d.jsx)(e.td,{children:"\u6309\u5b9e\u9645\u60c5\u51b5\uff0c\u9009\u62e9\u4ea4\u6362\u673aID\u3002\u82e5\u627e\u4e0d\u5230\u4ea4\u6362\u673a, \u53ef\u5c1d\u8bd5\u5207\u6362\u5730\u57df\u548c\u53ef\u7528\u533a"})]}),(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"ECS\u5b9e\u4f8b\u914d\u7f6e"}),(0,d.jsx)(e.td,{children:"\u5b9e\u4f8b\u7c7b\u578b"}),(0,d.jsx)(e.td,{children:"ecs.r6.xlarge"}),(0,d.jsx)(e.td,{children:"\u5f53\u524d\u652f\u6301ecs.r6.xlarge\u548cecs.r7a.xlarge\u89c4\u683c"})]}),(0,d.jsxs)(e.tr,{children:[(0,d.jsx)(e.td,{children:"ECS\u5b9e\u4f8b\u914d\u7f6e"}),(0,d.jsx)(e.td,{children:"\u5b9e\u4f8b\u5bc6\u7801"}),(0,d.jsx)(e.td,{children:"**"}),(0,d.jsxs)(e.td,{children:["\u8bbe\u7f6e\u5b9e\u4f8b\u5bc6\u7801\u3002\u957f\u5ea68",(0,d.jsx)(e.del,{children:"30\u4e2a\u5b57\u7b26\uff0c\u5fc5\u987b\u5305\u542b\u4e09\u9879\uff08\u5927\u5199\u5b57\u6bcd\u3001\u5c0f\u5199\u5b57\u6bcd\u3001\u6570\u5b57\u3001 ()`"}),"!@#$%^&*_-+={}[]:;'<>,.?/ \u4e2d\u7684\u7279\u6b8a\u7b26\u53f7\uff09\u3002"]})]})]})]}),"\n",(0,d.jsx)(e.h4,{id:"342\u5177\u4f53\u6b65\u9aa4",children:"3.4.2.\u5177\u4f53\u6b65\u9aa4"}),"\n",(0,d.jsx)(e.p,{children:"\u521b\u5efa\u670d\u52a1\u6309\u5982\u4e0b\u6b65\u9aa4\u8fdb\u884c\uff0c\u53c2\u8003\u4e0b\u56fe\uff1a"}),"\n",(0,d.jsxs)(e.ul,{children:["\n",(0,d.jsx)(e.li,{children:"\u521b\u5efa\u5b9e\u4f8b\u540d\u79f0\uff0c\u5982\u4e0b\u56fe\u4e2d\u201ctest\u201d"}),"\n",(0,d.jsx)(e.li,{children:"\u9009\u62e9\u5730\u57df\uff0c\u5982\u4e0b\u56fe\u4e2d\u201c\u534e\u4e1c1\uff08\u676d\u5dde\uff09\u201d"}),"\n"]}),"\n",(0,d.jsx)(e.p,{children:(0,d.jsx)(e.img,{alt:"\u521b\u5efa\u5b9e\u4f8b",src:s(58690).A+"",width:"2874",height:"1066"})}),"\n",(0,d.jsxs)(e.ul,{children:["\n",(0,d.jsx)(e.li,{children:"\u9009\u62e9\u5b9e\u4f8b\u7c7b\u578b\uff0c\u5f53\u524d\u652f\u6301ecs.r6.xlarge\u548cecs.r7a.xlarge\u89c4\u683c\u3002\u5982\u679c\u5217\u8868\u4e2d\u65e0\u673a\u578b\u53ef\u9009\uff0c\u8bf7\u5c1d\u8bd5\u9009\u62e9\u5176\u4ed6\u7684\u90e8\u7f72\u533a\u57df"}),"\n",(0,d.jsx)(e.li,{children:"\u9009\u4e2d\u673a\u578b"}),"\n",(0,d.jsx)(e.li,{children:"\u914d\u7f6e\u5b9e\u4f8b\u7684\u5bc6\u7801"}),"\n",(0,d.jsx)(e.li,{children:"\u9009\u62e9\u90e8\u7f72\u533a\u57df\uff0c\u5982\u4e0b\u56fe\u4e2d\u201c\u53ef\u7528\u533aI\u201d"}),"\n"]}),"\n",(0,d.jsx)(e.p,{children:(0,d.jsx)(e.img,{alt:"\u9009\u62e9\u533a\u57df",src:s(20413).A+"",width:"4102",height:"1242"})}),"\n",(0,d.jsxs)(e.ul,{children:["\n",(0,d.jsx)(e.li,{children:"\u70b9\u51fb\u4e0b\u4e00\u6b65\uff0c\u8fdb\u5165\u8ba2\u5355\u786e\u8ba4\u9875\u9762"}),"\n",(0,d.jsx)(e.li,{children:"\u52fe\u9009\u201c\u6743\u9650\u786e\u8ba4\u201d\u548c\u201c\u670d\u52a1\u6761\u6b3e\u201d\u4e2d\u7684\u590d\u9009\u6846"}),"\n",(0,d.jsx)(e.li,{children:"\u70b9\u51fb\u5de6\u4e0b\u89d2\u7eff\u8272\u80cc\u666f\u7684\u5f00\u59cb\u514d\u8d39\u8bd5\u7528\uff0c\u5373\u53ef\u521b\u5efa\u670d\u52a1\u5b9e\u4f8b"}),"\n"]}),"\n",(0,d.jsx)(e.p,{children:(0,d.jsx)(e.img,{alt:"\u786e\u8ba4",src:s(99284).A+"",width:"3414",height:"2180"})}),"\n",(0,d.jsx)(e.h3,{id:"35\u542f\u52a8tugraph\u670d\u52a1",children:"3.5.\u542f\u52a8TuGraph\u670d\u52a1"}),"\n",(0,d.jsxs)(e.ul,{children:["\n",(0,d.jsx)(e.li,{children:"\u67e5\u770b\u670d\u52a1\u5b9e\u4f8b\uff1a\u670d\u52a1\u5b9e\u4f8b\u521b\u5efa\u6210\u529f\u540e\uff0c\u90e8\u7f72\u65f6\u95f4\u5927\u7ea6\u9700\u89812\u5206\u949f\u3002\u90e8\u7f72\u5b8c\u6210\u540e\uff0c\u9875\u9762\u4e0a\u53ef\u4ee5\u770b\u5230\u5bf9\u5e94\u7684\u670d\u52a1\u5b9e\u4f8b\uff0c\u5982\u4e0b\u56fe"}),"\n"]}),"\n",(0,d.jsx)(e.p,{children:(0,d.jsx)(e.img,{alt:"\u67e5\u770b\u5b9e\u4f8b",src:s(4399).A+"",width:"4616",height:"1264"})}),"\n",(0,d.jsxs)(e.ul,{children:["\n",(0,d.jsx)(e.li,{children:"\u70b9\u51fb\u8be5\u670d\u52a1\u5b9e\u4f8b\u8bbf\u95eeTuGraph\u3002\u8fdb\u5165\u5230\u5bf9\u5e94\u7684\u670d\u52a1\u5b9e\u4f8b\u540e\uff0c\u53ef\u4ee5\u5728\u9875\u9762\u4e0a\u83b7\u53d6\u5230web\u3001rpc\u3001ssh\u51713\u79cd\u4f7f\u7528\u65b9\u5f0f\uff0c\u5e76\u4e14\u9875\u9762\u4e0a\u5c55\u793a\u4e86admin\u7528\u6237\u7684\u5bc6\u7801"}),"\n"]}),"\n",(0,d.jsx)(e.p,{children:(0,d.jsx)(e.img,{alt:"\u8bbf\u95ee\u65b9\u5f0f",src:s(39206).A+"",width:"1216",height:"612"})}),"\n",(0,d.jsxs)(e.ul,{children:["\n",(0,d.jsxs)(e.li,{children:["\u70b9\u51fbweb\u7684\u94fe\u63a5\uff0c\u5373\u53ef\u8df3\u8f6c\u8bbf\u95ee\u5df2\u7ecf\u90e8\u7f72\u597d\u7684TuGraph Web\u3002\u5efa\u8bae\u65b0\u624b\u5148\u901a\u8fc7TuGraph Web\uff0c\u5feb\u901f\u4f7f\u7528demo\u4e0a\u624b\u3002\n",(0,d.jsxs)(e.ul,{children:["\n",(0,d.jsx)(e.li,{children:"\u9996\u5148\u5728TuGraph Web\u7684\u767b\u5f55\u9875\u9762\u4e0a\uff0c\u8f93\u5165\u9ed8\u8ba4\u7528\u6237\u540dadmin\u548c\u9875\u9762\u4e0a\u5c55\u793a\u7684admin\u7528\u6237\u7684\u5bc6\u7801\u8fdb\u884c\u767b\u5f55\uff0c\u53c2\u8003\u4e0b\u56fe"}),"\n",(0,d.jsx)(e.li,{children:"\u767b\u5f55\u5b8c\u6210\u540e\uff0c\u9009\u62e9\u4efb\u610f\u4e00\u4e2a\u5e26\u6709\u5b98\u65b9\u56fe\u6807\u7684\u56fe\u9879\u76ee\uff0c\u5176\u4e2d\u5185\u7f6e\u4e86demo\u6570\u636e\uff0c\u5f00\u542f\u56fe\u6570\u636e\u7684\u63a2\u7d22\u548c\u53d1\u73b0\uff01"}),"\n"]}),"\n"]}),"\n"]}),"\n",(0,d.jsxs)(e.p,{children:[(0,d.jsx)(e.img,{alt:"\u767b\u5f55",src:s(50721).A+"",width:"1527",height:"1120"}),"\n",(0,d.jsx)(e.img,{alt:"\u521b\u5efademo",src:s(29592).A+"",width:"1709",height:"592"})]}),"\n",(0,d.jsx)(e.h2,{id:"4\u5e38\u89c1faq",children:"4.\u5e38\u89c1FAQ"}),"\n",(0,d.jsx)(e.h3,{id:"\u95ee\u9898\u4e00\u90e8\u7f72\u533a\u57df\u65e0\u53ef\u7528\u8d44\u6e90",children:"\u95ee\u9898\u4e00\uff1a\u90e8\u7f72\u533a\u57df\u65e0\u53ef\u7528\u8d44\u6e90"}),"\n",(0,d.jsx)(e.p,{children:"\u6709\u65f6\uff0c\u6240\u9009\u90e8\u7f72\u533a\u57df\uff08\u5982\u53ef\u7528\u533aG\uff09\u6ca1\u6709\u6240\u9009\u5957\u9910\u7684\u53ef\u7528\u8d44\u6e90\uff0c\u4f1a\u62a5\u9519\u5982\u4e0b\u56fe\u6240\u793a"}),"\n",(0,d.jsx)(e.p,{children:(0,d.jsx)(e.img,{alt:"\u90e8\u7f72\u533a\u57df\u9519\u8bef",src:s(71250).A+"",width:"596",height:"544"})}),"\n",(0,d.jsxs)(e.p,{children:[(0,d.jsx)(e.strong,{children:"\u89e3\u51b3\u529e\u6cd5"}),"\uff1a\u5c1d\u8bd5\u9009\u62e9\u5176\u4ed6\u533a\u57df\uff0c\u5982\u53ef\u7528\u533aI\u7b49"]}),"\n",(0,d.jsx)(e.h3,{id:"\u95ee\u9898\u4e8c-\u542f\u52a8\u540eweb\u8bbf\u95ee\u4e0d\u901a",children:"\u95ee\u9898\u4e8c: \u542f\u52a8\u540eweb\u8bbf\u95ee\u4e0d\u901a"}),"\n",(0,d.jsx)(e.p,{children:"web\u7684\u542f\u52a8\u9700\u8981\u4e00\u70b9\u70b9\u65f6\u95f4\uff0c\u8bf7\u7a0d\u540e\u5237\u65b0\u9875\u9762\u5373\u53ef\u3002"}),"\n",(0,d.jsx)(e.h3,{id:"\u95ee\u9898\u4e09-\u767b\u5f55\u65f6\u7684\u7528\u6237\u540d\u5bc6\u7801\u4e0d\u6b63\u786e",children:"\u95ee\u9898\u4e09: \u767b\u5f55\u65f6\u7684\u7528\u6237\u540d\u5bc6\u7801\u4e0d\u6b63\u786e"}),"\n",(0,d.jsx)(e.p,{children:"\u8bf7\u6ce8\u610f\u68c0\u67e5\uff0c\u767b\u5f55\u65f6\u4f7f\u7528\u7684\u5bc6\u7801\u5e94\u4e3a\u8be6\u60c5\u9875\u9762\u5c55\u793a\u7684\u5bc6\u7801\u3002"})]})}function x(n={}){const{wrapper:e}={...(0,l.R)(),...n.components};return e?(0,d.jsx)(e,{...n,children:(0,d.jsx)(a,{...n})}):a(n)}},58690:(n,e,s)=>{s.d(e,{A:()=>d});const d=s.p+"assets/images/cloud-deployment-3-72b2b61772400506bd9a80eb4d343017.png"},71250:(n,e,s)=>{s.d(e,{A:()=>d});const d=s.p+"assets/images/cloud-deployment-10-55820cf0e6bb8fac4c2e7d548b97a1fc.png"},99284:(n,e,s)=>{s.d(e,{A:()=>d});const d=s.p+"assets/images/cloud-deployment-5-f4512e5e080e0b9f6138224cc49d424e.png"}}]);