#!/bin/sh
ROUTECONF=pay_route.conf
AMOUNT=100000
PAY_BEGIN=4444
PAY_END=6666

PAYER=node_${PAY_BEGIN}
PAYER_PORT=$(( ${PAY_BEGIN} + 1 ))
PAYEE=node_${PAY_END}
PAYEE_PORT=$(( ${PAY_END} + 1 ))

echo invoiceのAMOUNTが異なるため、payeeノードで失敗する


nodeid() {
	cat conf/peer$1.conf | awk '(NR==3) { print $1 }' | cut -d '=' -f2
}

./routing $PAYER/dbucoin `nodeid $PAY_BEGIN` `nodeid $PAY_END` $AMOUNT
if [ $? -ne 0 ]; then
	echo no routing
	exit -1
fi

# invoiceだけ amountに 0を足して不一致にさせる
INVOICE=`./ucoincli -i ${AMOUNT}0 $PAYEE_PORT`
if [ $? -ne 0 ]; then
	echo fail get invoice
	exit -1
fi

echo -n hash= > $ROUTECONF
echo $INVOICE | jq -r '.result.hash' >> $ROUTECONF
./routing $PAYER/dbucoin `nodeid $PAY_BEGIN` `nodeid $PAY_END` $AMOUNT >> $ROUTECONF

# 送金実施
./ucoincli -p $ROUTECONF $PAYER_PORT
