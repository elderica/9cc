#!/bin/sh

cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x + y; }
int sub(int x, int y) { return x - y; }

int add6(int a, int b, int c, int d, int e, int f) {
    return a + b + c + d + e + f;
}
EOF

try() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    gcc -g -o tmp tmp.s tmp2.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

try 0 'main() { return 0; }'
try 42 'main() { return 42; }'
try 21 'main() { return 5+20-4; }'
try 41 'main() { return  12 + 34 - 5 ; }'
try 31 'main() { return      10 + 10 + 10   +  10    - 9; }'
try 47 'main() { return 5+6*7; }'
try 15 'main() { return 5*(9-6); }'
try 4 'main() { return (3+5)/2; }'
try 10 'main() { return -10+20; }'
try 89 'main() { return 63-(-13*2); }'
try 37 'main() { return 63-(-13*-2); }'
try 10 'main() { return - -10; }'
try 10 'main() { return - - +10; }'
try 1 'main() { return 1==1; }'
try 0 'main() { return 1==2; }'
try 0 'main() { return 1!=1; }'
try 1 'main() { return 1!=0; }'
try 1 'main() { return 1<2; }'
try 0 'main() { return 1<1; }'
try 0 'main() { return 2<1; }'
try 1 'main() { return 2>1; }'
try 0 'main() { return 1>1; }'
try 0 'main() { return 1>2; }'
try 1 'main() { return 1<=1; }'
try 1 'main() { return 1<=2; }'
try 0 'main() { return 2<=1; }'
try 1 'main() { return 1>=1; }'
try 1 'main() { return 2>=1; }'
try 0 'main() { return 1>=2; }'
try 1 'main() { return (1>=2)<1; }'
try 1 'main() { 3;2;return 1; }'
try 3 'main() { 1;2;return 3; }'
try 42 'main() { z=42;return z; }'
try 43 'main() { z=42;return z+1; }'
try 27 'main() { z=3*3;return z*3; }'
try 0 'main() { p=3;q=4;return p==q; }'
try 1 'main() { p=3;q=p;return p==q; }'
try 0 'main() { akizuki=1;haruzuki=9;return akizuki==haruzuki; }'
try 1 'main() { akizuki=1;haruzuki=9;return akizuki==akizuki; }'
try 1 'main() { akizuki=1;haruzuki=9;yoizuki=10;return haruzuki<=yoizuki; }'
try 1 'main() { akizuki=1;haruzuki=9;yoizuki=10;return haruzuki<=yoizuki; }'
try 1 'main() { p=3;q=p;return p==q;return p!=q; }'
try 42 'main() { p=3;q=p;return 42;return p==q;return p!=q; }'
try 27 'main() { a1=27;a2=20;return a1; }'
try 27 'main() { a_a=27;a_b=20;return a_a; }'
try 4 'main() { A=4;b=2;c3=5;return A; }'
try 3 'main() { if (0) return 2; return 3; }'
try 3 'main() { if (1-1) return 2; return 3; }'
try 2 'main() { if (1) return 2; return 3; }'
try 2 'main() { if (2-1) return 2; return 3; }'
try 27 'main() { if (0) return 18; else return 27; return 26; }'
try 18 'main() { if (1) return 18; else return 27; return 26; }'
try 3 'main() { if (0) return 1; if (2) return 3; else return 4; }'
try 10 'main() { i=0; while (i < 10) i=i+1; return i; }'
try 13 'main() { i=0; while (i <= 10) if (i / 5 == 1) return 13; else i = i+1; }'
try 55 'main() { i=0; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
try 3 'main() { for (;;) return 3; return 5; }'
try 5 'main() { i=1; m=0; for (; i<=15; i=i+1) if ((i-(i/3)*3)==0) m=m+1; return m; }'
try 3 'main() { {1; {2;} return 3;} }'
try 55 'main() { i=0; j=0; while (i<=10) { j=i+j; i=i+1; } return j; }'
try 3 'main() { return ret3(); }'
try 5 'main() { return ret5(); }'
try 8 'main() { return add(3, 5); }'
try 2 'main() { return sub(5, 3); }'
try 21 'main() { return add6(1, 2, 3, 4, 5, 6); }'
try 32 'main() { return ret32(); } ret32() { return 32; }'
try 6 'main() { return h(); } h() { return sub(9, 3); }'

echo OK

