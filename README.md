# Composable-List
a C++ Composable List, featuring both immutable and mutable variants, similar to Java.Function.andThen

# example

```cpp
int main() {

    #define M(v) [](int a) { std::cout << #v " " << std::to_string(a) << std::endl; return a+1; }

    {
        // a
        auto a = new Composable_List<int> (M( a ));
        a->visitFromStart(1);
        std::cout << std::endl;
        // a, b
        auto b = a->after(M( b ));
        b->visitFromStart(1);
        std::cout << std::endl;
        // b, a, b
        auto b1 = a->before(M( b ));
        b1->visitFromStart(1);
        std::cout << std::endl;
        // b, a, c, b
        auto c = a->after(M( c ));
        c->visitFromStart(1);
        std::cout << std::endl;
        // b, d, a, c, b
        auto d = a->before(M( d ));
        d->visitFromStart(1);
        std::cout << std::endl;
        // b, e, d, a, c, b
        auto e = d->before(M( e ));
        e->visitFromStart(1);
        std::cout << std::endl;
        // b, e, d, f, a, c, b
        auto f = d->after(M( f ));
        f->visitFromStart(1);
        std::cout << std::endl;
        // b, e, d, f, g, a, c, b
        auto g = f->after(M( g ));
        g->visitFromStart(1);
        std::cout << std::endl;
        // b, e, d, f, g, a, c, b
        b->visitFromStart(1);
    } {
        // a
        auto a = new Composable_List_COW<int> (M( a ));
        a->visitFromStart(2);
        std::cout << std::endl;
        // a, b
        auto b = a->after(M( b ));
        b->visitFromStart(2);
        std::cout << std::endl;
        // b, a, b
        auto b1 = b->before(M( b ));
        b1->visitFromStart(2);
        std::cout << std::endl;
        // b, a, c, b
        auto c = b1->after(a, M( c ));
        c->visitFromStart(2);
        std::cout << std::endl;
        // b, d, a, c, b
        auto d = c->before(a, M( d ));
        d->visitFromStart(2);
        std::cout << std::endl;
        // b, e, d, a, c, b
        auto e = d->before(d, M( e ));
        e->visitFromStart(2);
        std::cout << std::endl;
        // b, e, d, f, a, c, b
        auto f = e->after(d, M( f ));
        f->visitFromStart(2);
        std::cout << std::endl;
        // b, e, d, f, g, a, c, b
        auto g = f->after(f, M( g ));
        g->visitFromStart(2);
        std::cout << std::endl;
        // a, b
        b->visitFromStart(2);
    }

    #define M2(v) []() { std::cout << #v << std::endl; }

    {
        // a
        auto a = new Composable_List<void> (M2( a ));
        a->visitFromStart();
        std::cout << std::endl;
        // a, b
        auto b = a->after(M2( b ));
        b->visitFromStart();
        std::cout << std::endl;
        // b, a, b
        auto b1 = a->before(M2( b ));
        b1->visitFromStart();
        std::cout << std::endl;
        // b, a, c, b
        auto c = a->after(M2( c ));
        c->visitFromStart();
        std::cout << std::endl;
        // b, d, a, c, b
        auto d = a->before(M2( d ));
        d->visitFromStart();
        std::cout << std::endl;
        // b, e, d, a, c, b
        auto e = d->before(M2( e ));
        e->visitFromStart();
        std::cout << std::endl;
        // b, e, d, f, a, c, b
        auto f = d->after(M2( f ));
        f->visitFromStart();
        std::cout << std::endl;
        // b, e, d, f, g, a, c, b
        auto g = f->after(M2( g ));
        g->visitFromStart();
        std::cout << std::endl;
        // b, e, d, f, g, a, c, b
        b->visitFromStart();
    } {
        // a
        auto a = new Composable_List_COW<void> (M2( a ));
        a->visitFromStart();
        std::cout << std::endl;
        // a, b
        auto b = a->after(M2( b ));
        b->visitFromStart();
        std::cout << std::endl;
        // b, a, b
        auto b1 = b->before(M2( b ));
        b1->visitFromStart();
        std::cout << std::endl;
        // b, a, c, b
        auto c = b1->after(a, M2( c ));
        c->visitFromStart();
        std::cout << std::endl;
        // b, d, a, c, b
        auto d = c->before(a, M2( d ));
        d->visitFromStart();
        std::cout << std::endl;
        // b, e, d, a, c, b
        auto e = d->before(d, M2( e ));
        e->visitFromStart();
        std::cout << std::endl;
        // b, e, d, f, a, c, b
        auto f = e->after(d, M2( f ));
        f->visitFromStart();
        std::cout << std::endl;
        // b, e, d, f, g, a, c, b
        auto g = f->after(f, M2( g ));
        g->visitFromStart();
        std::cout << std::endl;
        // a, b
        b->visitFromStart();
    }

    return 0;
}
```
