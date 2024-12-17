"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[90772],{39914:(e,t,n)=>{n.r(t),n.d(t,{assets:()=>d,contentTitle:()=>o,default:()=>h,frontMatter:()=>i,metadata:()=>s,toc:()=>c});var a=n(74848),r=n(28453);const i={},o="Data Migration",s={id:"best-practices/data_migration",title:"Data Migration",description:"1 Introduction",source:"@site/versions/version-4.3.2/en-US/source/13.best-practices/3.data_migration.md",sourceDirName:"13.best-practices",slug:"/best-practices/data_migration",permalink:"/tugraph-db/en-US/en/4.3.2/best-practices/data_migration",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:3,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"Using TuGraph Graph Learning Module for Node Classification",permalink:"/tugraph-db/en-US/en/4.3.2/best-practices/learn_practices"},next:{title:"Environment and version selection",permalink:"/tugraph-db/en-US/en/4.3.2/best-practices/selection"}},d={},c=[{value:"1 Introduction",id:"1-introduction",level:2},{value:"2. Compatible Migration",id:"2-compatible-migration",level:2},{value:"2.1. Backup data",id:"21-backup-data",level:3},{value:"2.2. Start a new service",id:"22-start-a-new-service",level:3},{value:"2.3. Stop the original service",id:"23-stop-the-original-service",level:3},{value:"3. Upgrade migration",id:"3-upgrade-migration",level:2},{value:"3.1. Export data",id:"31-export-data",level:3},{value:"3.2. Import data",id:"32-import-data",level:3},{value:"3.3. Start a new service",id:"33-start-a-new-service",level:3},{value:"3.4. Stop the original service",id:"34-stop-the-original-service",level:3},{value:"4. Online Migration",id:"4-online-migration",level:2},{value:"4.1. Copy data",id:"41-copy-data",level:3},{value:"4.2. Starting a new node",id:"42-starting-a-new-node",level:3},{value:"4.3. Stop the original node",id:"43-stop-the-original-node",level:3}];function l(e){const t={a:"a",code:"code",h1:"h1",h2:"h2",h3:"h3",header:"header",li:"li",ol:"ol",p:"p",pre:"pre",...(0,r.R)(),...e.components};return(0,a.jsxs)(a.Fragment,{children:[(0,a.jsx)(t.header,{children:(0,a.jsx)(t.h1,{id:"data-migration",children:"Data Migration"})}),"\n",(0,a.jsx)(t.h2,{id:"1-introduction",children:"1 Introduction"}),"\n",(0,a.jsx)(t.p,{children:"Data migration refers to the process of moving data from one system, storage medium, or application to another system, storage medium, or application. When TuGraph needs to be upgraded or the system hardware environment changes,\nThe data in the original TuGraph service needs to be migrated. Based on the system hardware environment and software version, this paper divides data migration into three schemes:"}),"\n",(0,a.jsxs)(t.ol,{children:["\n",(0,a.jsx)(t.li,{children:"Compatible migration: When the system environment before and after the migration is consistent and the TuGraph software is compatible, you can directly use the backup and recovery method to migrate data;"}),"\n",(0,a.jsx)(t.li,{children:"Upgrade and migration: When the system environment before and after the migration is inconsistent or the TuGraph software is not compatible, it is necessary to migrate the data by first exporting the data and then re-importing;"}),"\n",(0,a.jsx)(t.li,{children:"Online migration: When data migration is performed on a high-availability cluster and the network environment of the cluster is good, the original cluster can be smoothly switched to the new cluster by adding or deleting nodes.\nThe following article will introduce these three schemes in detail."}),"\n"]}),"\n",(0,a.jsx)(t.h2,{id:"2-compatible-migration",children:"2. Compatible Migration"}),"\n",(0,a.jsxs)(t.p,{children:["Compatible migration means that when the system environment remains unchanged and the TuGraph software version is compatible, the data and stored procedures of the original service can be used in the new service, so it can be directly migrated.\nUsers can first use the ",(0,a.jsx)(t.code,{children:"lgraph_backup"})," tool to back up the data, then transfer the data to a new machine and restart the service. The specific migration steps are as follows:"]}),"\n",(0,a.jsx)(t.h3,{id:"21-backup-data",children:"2.1. Backup data"}),"\n",(0,a.jsxs)(t.p,{children:["Backup data using ",(0,a.jsx)(t.code,{children:"lgraph_backup"})," tool"]}),"\n",(0,a.jsx)(t.pre,{children:(0,a.jsx)(t.code,{className:"language-bash",children:"   lgraph_backup -s db -d db.bck\n"})}),"\n",(0,a.jsxs)(t.p,{children:["You can also directly use the ",(0,a.jsx)(t.code,{children:"cp"})," command in this step, but the ",(0,a.jsx)(t.code,{children:"cp"})," command will copy some redundant metadata, and the raft metadata will also be copied in the HA mode, causing the cluster to fail to restart after migration.\nTherefore, it is recommended to use the ",(0,a.jsx)(t.code,{children:"lgraph_backup"})," tool instead of the ",(0,a.jsx)(t.code,{children:"cp"})," command during data migration."]}),"\n",(0,a.jsx)(t.h3,{id:"22-start-a-new-service",children:"2.2. Start a new service"}),"\n",(0,a.jsx)(t.p,{children:"Use the following command to start the new service, and the stored procedure will be automatically loaded into the new service"}),"\n",(0,a.jsx)(t.pre,{children:(0,a.jsx)(t.code,{className:"language-bash",children:"   lgraph_server -c /usr/local/etc/lgraph.json --directory db.bck -d start\n"})}),"\n",(0,a.jsx)(t.h3,{id:"23-stop-the-original-service",children:"2.3. Stop the original service"}),"\n",(0,a.jsx)(t.p,{children:"Use the following command to stop the original service"}),"\n",(0,a.jsx)(t.pre,{children:(0,a.jsx)(t.code,{className:"language-bash",children:"   lgraph_server -c /usr/local/etc/lgraph.json --directory db.bck -d stop\n"})}),"\n",(0,a.jsx)(t.h2,{id:"3-upgrade-migration",children:"3. Upgrade migration"}),"\n",(0,a.jsxs)(t.p,{children:["When the user wants to migrate the original service to a differentiated environment (such as migrating from centos7 to ubuntu18.04), or when the version of TuGraph changes greatly and is incompatible (such as 3.4.0 and 3.6.0),\nUsers can first use the ",(0,a.jsx)(t.code,{children:"lgraph_export"})," tool to export the data into a file, transfer it to a new machine, and then use the ",(0,a.jsx)(t.code,{children:"lgraph_import"})," tool to re-import and restart the cluster.\nThis can ensure that it can be used in the new environment, but the efficiency is low, and the stored procedure needs to be reloaded. The specific migration steps are as follows:"]}),"\n",(0,a.jsx)(t.h3,{id:"31-export-data",children:"3.1. Export data"}),"\n",(0,a.jsxs)(t.p,{children:["Use the ",(0,a.jsx)(t.code,{children:"lgraph_export"})," tool to export the data and transfer the data to the new machine"]}),"\n",(0,a.jsx)(t.pre,{children:(0,a.jsx)(t.code,{className:"language-bash",children:"   lgraph_export -d db -e db.export\n"})}),"\n",(0,a.jsx)(t.h3,{id:"32-import-data",children:"3.2. Import data"}),"\n",(0,a.jsxs)(t.p,{children:["Use the ",(0,a.jsx)(t.code,{children:"lgraph_import"})," tool to import data and manually load the stored procedure (see ",(0,a.jsx)(t.a,{href:"/tugraph-db/en-US/en/4.3.2/client-tools/cpp-client",children:"client operation steps"})," for details)"]}),"\n",(0,a.jsx)(t.pre,{children:(0,a.jsx)(t.code,{className:"language-bash",children:"   lgraph_import -c db.export/import.config -d db\n"})}),"\n",(0,a.jsx)(t.h3,{id:"33-start-a-new-service",children:"3.3. Start a new service"}),"\n",(0,a.jsx)(t.p,{children:"Start the new service with the following command"}),"\n",(0,a.jsx)(t.pre,{children:(0,a.jsx)(t.code,{className:"language-bash",children:"    lgraph_server -c /usr/local/etc/lgraph.json --directory db.export -d start\n"})}),"\n",(0,a.jsx)(t.h3,{id:"34-stop-the-original-service",children:"3.4. Stop the original service"}),"\n",(0,a.jsx)(t.p,{children:"Use the following command to stop the original service"}),"\n",(0,a.jsx)(t.pre,{children:(0,a.jsx)(t.code,{className:"language-bash",children:"    lgraph_server -c /usr/local/etc/lgraph.json --directory db.export -d stop\n"})}),"\n",(0,a.jsx)(t.h2,{id:"4-online-migration",children:"4. Online Migration"}),"\n",(0,a.jsx)(t.p,{children:"When performing data migration on the server cluster deployed by the high-availability version of TuGraph, if the network bandwidth is sufficient, you can directly migrate the service online by adding or deleting nodes. The specific migration steps are as follows:"}),"\n",(0,a.jsx)(t.h3,{id:"41-copy-data",children:"4.1. Copy data"}),"\n",(0,a.jsx)(t.p,{children:"Use the following commands to copy the data on the leader node and transfer it to the machine nodes of the new cluster. Since the leader node has the most complete raft log, copying the leader's data can minimize\nThe time for the log to catch up."}),"\n",(0,a.jsx)(t.pre,{children:(0,a.jsx)(t.code,{className:"language-bash",children:"   cp -r db db.cp\n"})}),"\n",(0,a.jsx)(t.h3,{id:"42-starting-a-new-node",children:"4.2. Starting a new node"}),"\n",(0,a.jsx)(t.p,{children:"Use the following command to join the new node to the cluster. After joining the cluster, the incremental data will be automatically synchronized to the new node"}),"\n",(0,a.jsx)(t.pre,{children:(0,a.jsx)(t.code,{className:"language-bash",children:"   lgraph_server -c /usr/local/etc/lgraph_ha.json --directory db.cp --ha_conf 192.168.0.1:9090,192.168.0.2:9090,192.168.0.3:9090 -d start\n"})}),"\n",(0,a.jsx)(t.h3,{id:"43-stop-the-original-node",children:"4.3. Stop the original node"}),"\n",(0,a.jsx)(t.p,{children:"Stop the original node service, and send subsequent application requests directly to the new cluster"}),"\n",(0,a.jsx)(t.pre,{children:(0,a.jsx)(t.code,{className:"language-bash",children:"   lgraph_server -c /usr/local/etc/lgraph_ha.json --directory db.cp --ha_conf 192.168.0.1:9090,192.168.0.2:9090,192.168.0.3:9090 -d stop\n"})})]})}function h(e={}){const{wrapper:t}={...(0,r.R)(),...e.components};return t?(0,a.jsx)(t,{...e,children:(0,a.jsx)(l,{...e})}):l(e)}},28453:(e,t,n)=>{n.d(t,{R:()=>o,x:()=>s});var a=n(96540);const r={},i=a.createContext(r);function o(e){const t=a.useContext(i);return a.useMemo((function(){return"function"==typeof e?e(t):{...t,...e}}),[t,e])}function s(e){let t;return t=e.disableParentContext?"function"==typeof e.components?e.components(r):e.components||r:o(e.components),a.createElement(i.Provider,{value:t},e.children)}}}]);