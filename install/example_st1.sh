#!/bin/sh

# 1台のPCで複数ノードを立ち上げるので、ディレクトリをそれぞれ作る。
# 起動したディレクトリに関連ファイルを作成するためだ。
mkdir -p node_3333
mkdir -p node_4444
mkdir -p node_5555

# ノード設定ファイルを作成する。
# 引数はポート番号で、そのポート番号で相手を待ち受ける。
# このファイルにはノードの秘密鍵が書かれているので、人には渡さない。
#
# 作成したノード設定ファイルは、ucoindの引数に与える。
./create_nodeconf.sh 3333 > node_3333/node.conf
./create_nodeconf.sh 4444 > node_4444/node.conf
./create_nodeconf.sh 5555 > node_5555/node.conf

# ピア設定ファイルを作成する。
# 自ノードが相手のノードと接続するための設定が書かれている。
# IPアドレス、ポート番号、ノードID(ノード秘密鍵に対する公開鍵)が書かれている。
#
# このファイルを接続したい相手に渡す。
# 相手ノードは、「ucoincli -c <ピア設定ファイル>」の形で接続相手を指定する。
mkdir -p conf
./create_peerconf.sh node_3333/node.conf > conf/peer3333.conf
./create_peerconf.sh node_4444/node.conf > conf/peer4444.conf
./create_peerconf.sh node_5555/node.conf > conf/peer5555.conf