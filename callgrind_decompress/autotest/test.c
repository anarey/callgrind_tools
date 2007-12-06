int fun1(){
    int x = 0;
    int y = 1;
    int z = x + y;
    return z;
}

int fun2(){
    fun1();
}

int main() {
    fun1();
    fun2();
    return 0;
}

