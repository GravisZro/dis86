#include "dvec.h"

#ifdef ENABLE_TESTS
#include <cassert>
#include "common/print.h"

namespace dvec
{
  void test(void)
  {
    DVec<int> d;
    assert(d.start() == 0);
    assert(d.end() == 0);

    d.push_back(4);
    assert(d.start() == 0);
    assert(d.end() == 1);

    d.push_front(3);
    assert(d.start() == -1);
    assert(d.end() == 1);

    d.push_front(5);
    assert(d.start() == -2);
    assert(d.end() == 1);
/*
    let idx: Vec<_> = d.range().collect();
    assert(idx, vec![-2, -1, 0]);

    let elts: Vec<_> = d.range().map(|i| d[i]).collect();
    assert(elts, vec![5, 3, 4]);

    d[-1] = 42;
    let elts: Vec<_> = d.range().map(|i| d[i]).collect();
    assert(elts, vec![5, 42, 4]);
*/
    println("dvec tests passed");
  }

}
#endif
