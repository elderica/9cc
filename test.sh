#!/bin/sh

try() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

try 0 '0;'
try 42 '42;'
try 21 '5+20-4;'
try 41 ' 12 + 34 - 5 ;'
try 31 '     10 + 10 + 10   +  10    - 9;'
#try 3 '1+2g'
try 47 '5+6*7;'
try 15 '5*(9-6);'
try 4 '(3+5)/2;'
try 10 '-10+20;'
try 89 '63-(-13*2);'
try 37 '63-(-13*-2);'
try 10 '- -10;'
try 10 '- - +10;'
try 1 '1==1;'
try 0 '1==2;'
try 0 '1!=1;'
try 1 '1!=0;'
try 1 '1<2;'
try 0 '1<1;'
try 0 '2<1;'
try 1 '2>1;'
try 0 '1>1;'
try 0 '1>2;'
try 1 '1<=1;'
try 1 '1<=2;'
try 0 '2<=1;'
try 1 '1>=1;'
try 1 '2>=1;'
try 0 '1>=2;'
try 1 '(1>=2)<1;'
try 1 '3;2;1;'
try 3 '1;2;3;'
try 42 'z=42;z;'
try 43 'z=42;z+1;'
try 27 'z=3*3;z*3;'
try 0 'p=3;q=4;p==q;'
try 1 'p=3;q=p;p==q;'
try 0 'akizuki=1;haruzuki=9;akizuki==haruzuki;'
try 1 'akizuki=1;haruzuki=9;akizuki==akizuki;'
try 1 'akizuki=1;haruzuki=9;yoizuki=10;haruzuki<=yoizuki;'
try 1 'akizuki=1;haruzuki=9;yoizuki=10;haruzuki<=yoizuki;'
try 1 'p=3;q=p;return p==q;p!=q;'
try 42 'p=3;q=p;return 42;return p==q;p!=q;'

echo OK

