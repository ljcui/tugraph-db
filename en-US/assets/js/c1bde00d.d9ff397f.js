"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[23898],{7187:(e,n,t)=>{t.r(n),t.d(n,{assets:()=>d,contentTitle:()=>s,default:()=>c,frontMatter:()=>o,metadata:()=>i,toc:()=>l});var r=t(74848),a=t(28453);const o={},s="Using TuGraph Graph Learning Module for Node Classification",i={id:"best-practices/learn_practices",title:"Using TuGraph Graph Learning Module for Node Classification",description:"1. Introduction",source:"@site/versions/version-3.6.0/en-US/source/7.best-practices/2.learn_practices.md",sourceDirName:"7.best-practices",slug:"/best-practices/learn_practices",permalink:"/tugraph-db/en-US/en/3.6.0/best-practices/learn_practices",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:2,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"Importing Data from Relational Databases to TuGraph",permalink:"/tugraph-db/en-US/en/3.6.0/best-practices/rdbms-to-tugraph"},next:{title:"FAQ",permalink:"/tugraph-db/en-US/en/3.6.0/faq"}},d={},l=[{value:"1. Introduction",id:"1-introduction",level:2},{value:"2. Prerequisites",id:"2-prerequisites",level:2},{value:"3. Import Cora Dataset to TuGraph Database",id:"3-import-cora-dataset-to-tugraph-database",level:2},{value:"3.1. Introduction to Cora Dataset",id:"31-introduction-to-cora-dataset",level:3},{value:"3.2. Data Import",id:"32-data-import",level:3},{value:"4. Feature Conversion",id:"4-feature-conversion",level:2},{value:"5. Compile Sampling Operator",id:"5-compile-sampling-operator",level:2},{value:"6. Training Process",id:"6-training-process",level:2},{value:"6.1. Data Loading",id:"61-data-loading",level:3},{value:"6.2. Build Sampler",id:"62-build-sampler",level:3},{value:"6.3. Convert the results to the required format",id:"63-convert-the-results-to-the-required-format",level:3},{value:"6.4. Build the GCN model",id:"64-build-the-gcn-model",level:3},{value:"6.5. Train the GCN model",id:"65-train-the-gcn-model",level:3}];function h(e){const n={a:"a",code:"code",h1:"h1",h2:"h2",h3:"h3",header:"header",p:"p",pre:"pre",table:"table",tbody:"tbody",td:"td",th:"th",thead:"thead",tr:"tr",...(0,a.R)(),...e.components};return(0,r.jsxs)(r.Fragment,{children:[(0,r.jsx)(n.header,{children:(0,r.jsx)(n.h1,{id:"using-tugraph-graph-learning-module-for-node-classification",children:"Using TuGraph Graph Learning Module for Node Classification"})}),"\n",(0,r.jsx)(n.h2,{id:"1-introduction",children:"1. Introduction"}),"\n",(0,r.jsx)(n.p,{children:"GNN is a powerful tool for many machine learning tasks on graphs. In this introductory tutorial, you will learn the basic workflow of using GNN for node classification, i.e., predicting the category of nodes in a graph."}),"\n",(0,r.jsx)(n.p,{children:"This document will show how to build a GNN for semi-supervised node classification on the Cora dataset with only a few labels, which is a citation network with papers as nodes and citations as edges. The task is to predict the category of a given paper.\nBy completing this tutorial, you will be able to:"}),"\n",(0,r.jsx)(n.p,{children:"Load the Cora dataset."}),"\n",(0,r.jsx)(n.p,{children:"Sample and build a GNN model using the sampling operator provided by TuGraph.\nTrain the GNN model for node classification on CPU or GPU."}),"\n",(0,r.jsx)(n.p,{children:"This document requires some experience in using graph neural networks and DGL."}),"\n",(0,r.jsx)(n.h2,{id:"2-prerequisites",children:"2. Prerequisites"}),"\n",(0,r.jsx)(n.p,{children:"The TuGraph graph learning module requires TuGraph-db version 3.5.1 or above."}),"\n",(0,r.jsx)(n.p,{children:"It is recommended to use Docker image tugraph-compile 1.2.4 or above for TuGraph deployment:"}),"\n",(0,r.jsx)(n.p,{children:"tugraph/tugraph-compile-ubuntu18.04:latest"}),"\n",(0,r.jsx)(n.p,{children:"tugraph/tugraph-compile-centos7:latest"}),"\n",(0,r.jsx)(n.p,{children:"tugraph/tugraph-compile-centos8:latest"}),"\n",(0,r.jsxs)(n.p,{children:["The above images can be obtained on DockerHub.\nPlease refer to ",(0,r.jsx)(n.a,{href:"/tugraph-db/en-US/en/3.6.0/quick-start/preparation",children:"Quick Start"})," for specific operations."]}),"\n",(0,r.jsx)(n.h2,{id:"3-import-cora-dataset-to-tugraph-database",children:"3. Import Cora Dataset to TuGraph Database"}),"\n",(0,r.jsx)(n.h3,{id:"31-introduction-to-cora-dataset",children:"3.1. Introduction to Cora Dataset"}),"\n",(0,r.jsx)(n.p,{children:"The Cora dataset consists of 2708 papers, classified into 7 categories. Each paper is represented by a 1433-dimensional bag-of-words feature, indicating whether a certain word appears in the paper. These bag-of-words features have been preprocessed to be normalized to a range from 0 to 1. The edges represent citation relationships between papers."}),"\n",(0,r.jsx)(n.p,{children:"TuGraph has provided the Cora dataset import tool, which users can use directly."}),"\n",(0,r.jsx)(n.h3,{id:"32-data-import",children:"3.2. Data Import"}),"\n",(0,r.jsx)(n.p,{children:"The Cora dataset is located in the test/integration/data/algo directory and contains the cora_vertices point set and the cora_edge edge set."}),"\n",(0,r.jsxs)(n.p,{children:["First, the Cora dataset needs to be imported into the TuGraph database. Please refer to ",(0,r.jsx)(n.a,{href:"/tugraph-db/en-US/en/3.6.0/developer-manual/server-tools/data-import",children:"Data Import"}),"."]}),"\n",(0,r.jsx)(n.p,{children:"Execute the following command in the build/output directory:"}),"\n",(0,r.jsx)(n.pre,{children:(0,r.jsx)(n.code,{className:"language-bash",children:"./lgraph_import -c ./../../test/integration/data/algo/cora.conf --dir ./cora_db --overwrite true\n"})}),"\n",(0,r.jsx)(n.p,{children:"Here, cora.conf is the graph schema file representing the format of graph data, which can be found in test/integration/data/algo/cora.conf. cora_db is the name of the graph data file after import, which represents the storage location of graph data."}),"\n",(0,r.jsx)(n.h2,{id:"4-feature-conversion",children:"4. Feature Conversion"}),"\n",(0,r.jsx)(n.p,{children:"Since the features in the Cora dataset are float arrays of length 1433, which are not supported by TuGraph for loading, they can be imported as strings and converted to char* for easier access later. The implementation can be referred to the feature_float.cpp file.\nThe specific execution process is as follows:"}),"\n",(0,r.jsxs)(n.p,{children:["Compile the import plugin in the build directory:\n",(0,r.jsx)(n.code,{children:"make feature_float_embed"})]}),"\n",(0,r.jsxs)(n.p,{children:["Execute the following command in the build/output directory to perform the conversion:\n",(0,r.jsx)(n.code,{children:"./algo/feature_float_embed ./cora_db"})]}),"\n",(0,r.jsx)(n.h2,{id:"5-compile-sampling-operator",children:"5. Compile Sampling Operator"}),"\n",(0,r.jsxs)(n.p,{children:["The sampling operator is used to obtain graph data from the database and convert it into the required data structure. The specific execution process is as follows:\nExecute the following command in the tugraph-db/build directory:\n",(0,r.jsx)(n.code,{children:"make -j2"})]}),"\n",(0,r.jsxs)(n.p,{children:["or execute the following command in the tugraph-db/learn/procedures directory:\n",(0,r.jsx)(n.code,{children:"python3 setup.py build_ext -i"})]}),"\n",(0,r.jsx)(n.pre,{children:(0,r.jsx)(n.code,{className:"language-python",children:'from lgraph_db_python import *\nimport importlib\ngetdb = importlib.import_module("getdb")\ngetdb.Process(db, olapondb, feature_len, NodeInfo, EdgeInfo)\n'})}),"\n",(0,r.jsx)(n.p,{children:"As shown in the code, after obtaining the algorithm .so file, import and use it."}),"\n",(0,r.jsx)(n.h2,{id:"6-training-process",children:"6. Training Process"}),"\n",(0,r.jsx)(n.h3,{id:"61-data-loading",children:"6.1. Data Loading"}),"\n",(0,r.jsx)(n.pre,{children:(0,r.jsx)(n.code,{className:"language-python",children:"galaxy = PyGalaxy(args.db_path)\ngalaxy.SetCurrentUser(args.username, args.password)\ndb = galaxy.OpenGraph(args.graph_name, False)\n"})}),"\n",(0,r.jsx)(n.p,{children:"As shown in the code, load the data into memory based on the path of the graph data, username, password, and subgraph name. TuGraph can load multiple subgraphs for graph training, but we only load one subgraph here."}),"\n",(0,r.jsx)(n.h3,{id:"62-build-sampler",children:"6.2. Build Sampler"}),"\n",(0,r.jsx)(n.p,{children:"During the training process, the GetDB operator is first used to obtain the graph data from the database and convert it into the required data structure. The specific code is as follows:"}),"\n",(0,r.jsx)(n.pre,{children:(0,r.jsx)(n.code,{className:"language-python",children:"    GetDB.Process(db_: lgraph_db_python.PyGraphDB, olapondb: lgraph_db_python.PyOlapOnDB, feature_num: size_t, NodeInfo: list, EdgeInfo: list)\n"})}),"\n",(0,r.jsx)(n.p,{children:"As shown in the code, the results are stored in NodeInfo and EdgeInfo. NodeInfo and EdgeInfo are python list results, and their stored information is as follows:"}),"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",(0,r.jsxs)(n.table,{children:[(0,r.jsx)(n.thead,{children:(0,r.jsxs)(n.tr,{children:[(0,r.jsx)(n.th,{children:"Graph Data"}),(0,r.jsx)(n.th,{children:"Storage Position"})]})}),(0,r.jsxs)(n.tbody,{children:[(0,r.jsxs)(n.tr,{children:[(0,r.jsx)(n.td,{children:"Edge source"}),(0,r.jsx)(n.td,{children:"EdgeInfo[0]"})]}),(0,r.jsxs)(n.tr,{children:[(0,r.jsx)(n.td,{children:"Edge destination"}),(0,r.jsx)(n.td,{children:"EdgeInfo[1]"})]}),(0,r.jsxs)(n.tr,{children:[(0,r.jsx)(n.td,{children:"Vertex ID"}),(0,r.jsx)(n.td,{children:"NodeInfo[0]"})]}),(0,r.jsxs)(n.tr,{children:[(0,r.jsx)(n.td,{children:"Vertex features"}),(0,r.jsx)(n.td,{children:"NodeInfo[1]"})]}),(0,r.jsxs)(n.tr,{children:[(0,r.jsx)(n.td,{children:"Vertex label"}),(0,r.jsx)(n.td,{children:"NodeInfo[2]"})]})]})]}),"\n",(0,r.jsx)(n.p,{children:"Then a sampler is constructed:"}),"\n",(0,r.jsx)(n.pre,{children:(0,r.jsx)(n.code,{className:"language-python",children:"batch_size = 5\ncount = 2708\nsampler = TugraphSample(args)\ndataloader = dgl.dataloading.DataLoader(fake_g,\n    torch.arange(count),\n    sampler,\n    batch_size=batch_size,\n    num_workers=0,\n    )\n"})}),"\n",(0,r.jsx)(n.h3,{id:"63-convert-the-results-to-the-required-format",children:"6.3. Convert the results to the required format"}),"\n",(0,r.jsx)(n.pre,{children:(0,r.jsx)(n.code,{className:"language-python",children:"src = EdgeInfo[0].astype('int64')\ndst = EdgeInfo[1].astype('int64')\nnodes_idx = NodeInfo[0].astype('int64')\nremap(src, dst, nodes_idx)\nfeatures = NodeInfo[1].astype('float32')\nlabels = NodeInfo[2].astype('int64')\ng = dgl.graph((src, dst))\ng.ndata['feat'] = torch.tensor(features)\ng.ndata['label'] = torch.tensor(labels)\nreturn g\n"})}),"\n",(0,r.jsx)(n.p,{children:"The results are converted to the required format to fit the training format."}),"\n",(0,r.jsx)(n.h3,{id:"64-build-the-gcn-model",children:"6.4. Build the GCN model"}),"\n",(0,r.jsx)(n.pre,{children:(0,r.jsx)(n.code,{className:"language-python",children:"class GCN(nn.Module):\n    def __init__(self, in_size, hid_size, out_size):\n        super().__init__()\n        self.layers = nn.ModuleList()\n        # two-layer GCN\n        self.layers.append(dgl.nn.GraphConv(in_size, hid_size, activation=F.relu))\n        self.layers.append(dgl.nn.GraphConv(hid_size, out_size))\n        self.dropout = nn.Dropout(0.5)\n    def forward(self, g, features):\n        h = features\n        for i, layer in enumerate(self.layers):\n            if i != 0:\n                h = self.dropout(h)\n            h = layer(g, h)\n        return h\ndef build_model():\n    in_size = feature_len  # feature_len is the length of features, which is 1433 here\n    out_size = classes  # classes is the number of classes, which is 7 here\n    model = GCN(in_size, 16, out_size)  # 16 is the size of the hidden layer\n    return model\n"})}),"\n",(0,r.jsx)(n.p,{children:"In this tutorial, a two-layer Graph Convolutional Network (GCN) will be built. Each layer aggregates neighbor information to compute new node representations."}),"\n",(0,r.jsx)(n.h3,{id:"65-train-the-gcn-model",children:"6.5. Train the GCN model"}),"\n",(0,r.jsx)(n.pre,{children:(0,r.jsx)(n.code,{className:"language-python",children:"loss_fcn = nn.CrossEntropyLoss()\ndef train(graph, model):\n    optimizer = torch.optim.Adam(model.parameters(), lr=1e-2, weight_decay=5e-4)\n    model.train()\n    s = time.time()\n    load_time = time.time()\n    graph = dgl.add_self_loop(graph)\n    logits = model(graph, graph.ndata['feat'])\n    loss = loss_fcn(logits, graph.ndata['label'])\n    optimizer.zero_grad()\n    loss.backward()\n    optimizer.step()\n    train_time = time.time()\n    return float(loss)\nfor epoch in range(50):\n    model.train()\n    total_loss = 0\n    loss = train(g, model)\n    if epoch % 5 == 0:\n        print('In epoch', epoch, ', loss', loss)\n    sys.stdout.flush()\n"})}),"\n",(0,r.jsx)(n.p,{children:"As shown in the code, iterate and train 50 times based on the defined sampler, optimizer, and model."}),"\n",(0,r.jsx)(n.p,{children:"The output is as follows:"}),"\n",(0,r.jsx)(n.pre,{children:(0,r.jsx)(n.code,{className:"language-bash",children:"In epoch 0 , loss 1.9586775302886963\nIn epoch 5 , loss 1.543689250946045\nIn epoch 10 , loss 1.160698413848877\nIn epoch 15 , loss 0.8862786889076233\nIn epoch 20 , loss 0.6973256468772888\nIn epoch 25 , loss 0.5770673751831055\nIn epoch 30 , loss 0.5271289348602295\nIn epoch 35 , loss 0.45514997839927673\nIn epoch 40 , loss 0.43748989701271057\nIn epoch 45 , loss 0.3906335234642029\n"})}),"\n",(0,r.jsx)(n.p,{children:"The graph learning module can be accelerated by using a GPU. If users want to run it on the GPU, they need to install the corresponding GPU driver and environment. For details, please refer to learn/README.md."}),"\n",(0,r.jsx)(n.p,{children:"The complete code can be found in learn/examples/train_full_cora.py."})]})}function c(e={}){const{wrapper:n}={...(0,a.R)(),...e.components};return n?(0,r.jsx)(n,{...e,children:(0,r.jsx)(h,{...e})}):h(e)}},28453:(e,n,t)=>{t.d(n,{R:()=>s,x:()=>i});var r=t(96540);const a={},o=r.createContext(a);function s(e){const n=r.useContext(o);return r.useMemo((function(){return"function"==typeof e?e(n):{...n,...e}}),[n,e])}function i(e){let n;return n=e.disableParentContext?"function"==typeof e.components?e.components(a):e.components||a:s(e.components),r.createElement(o.Provider,{value:n},e.children)}}}]);