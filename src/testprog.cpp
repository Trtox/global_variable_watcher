long long global_var = 0;

int main() {
    for (int i = 0; i < 100000; i++) {
        global_var++;
    }
    return 0;
}