'use strict';

const {
  Array,
  Symbol,
} = primordials;

const kCompare = Symbol('compare');
const kHeap = Symbol('heap');
const kSetPosition = Symbol('setPosition');
const kSize = Symbol('size');

module.exports = class PriorityQueue {
  // 比较函数和节点移动位置后的回调函数
  constructor(comparator, setPosition) {
    if (comparator !== undefined) {
      this[kCompare] = comparator;
    }

    if (setPosition !== undefined) {
      this[kSetPosition] = setPosition;
    }

    // 用一个数组保存二叉堆的节点
    this[kHeap] = new Array(64);
    // 堆中的元素个数
    this[kSize] = 0;
  }

  // 默认的比较函数
  [kCompare](a, b) {
    return a - b;
  }

  // 插入堆
  insert (value) {
    const heap = this[kHeap];
    // 为了计算方便，从1开始存储数据
    const pos = ++this[kSize];

    heap[pos] = value;

    // 扩容
    if (heap.length === pos) {
      heap.length *= 2;
    }

    // 把元素存在最后一个叶子节点，往上冒
    this.percolateUp(pos);
  }

  // 取根节点
  peek () {
    return this[kHeap][1];
  }

  // pos代表那个元素需要往下沉
  percolateDown (pos) {
    const compare = this[kCompare];
    const setPosition = this[kSetPosition];
    const heap = this[kHeap];
    const size = this[kSize];
    const item = heap[pos];
    /*
      从需要下沉的节点开始，调整子树，size为元素个数，
      pos*2小于等于size说明pos位置的元素还有孩子，即还没沉到底
    */
    while (pos * 2 <= size) {
      // 右孩子
      let childIndex = pos * 2 + 1;
      // childIndex > size说明没有右孩子，只有左孩子。否则说明有右孩子，则比较左右孩子，小于0说明右孩子大，则取值小的
      if (childIndex > size || compare(heap[pos * 2], heap[childIndex]) < 0)
        childIndex = pos * 2;
      /*
        拿到值小的节点和父节点比较，一旦需要交换位置的话，也满足二叉堆。否则
        如何和大的节点比较，同时大的节点满足上升的话，新的根节点比孩子大。
              4                         2                               3
        比如2   3，4要和2比，2上升变成4    3满足二叉堆，如果和3比则变成2    4，不满足二叉堆规则
      */
      const child = heap[childIndex];
      // 比较值小的节点和当前需要下沉的节点，如果父节点比字节的值大，则满足二叉堆规则，不需要继续调整了
      if (compare(item, child) <= 0)
        break;
      // 否则说明父节点比子节点值小，更新子节点的位置信息
      if (setPosition !== undefined)
        setPosition(child, pos);
      // 子节点往上冒，子节点的位置空闲
      heap[pos] = child;
      // 继续调整子节点为根的子树
      pos = childIndex;
    }

    // pos就是item新的位置
    heap[pos] = item;

    if (setPosition !== undefined) {
      setPosition(item, pos);
    }
  }
  // pos代表那个元素需要往上冒
  percolateUp(pos) {
    const heap = this[kHeap];
    const compare = this[kCompare];
    const setPosition = this[kSetPosition];
    const item = heap[pos];
    // 大于1，根节点不需要往上冒了
    while (pos > 1) {
      // 完全二叉树，父和子的关系是子等于父索引*2和父索引*2加一
      const parent = heap[pos / 2 | 0];
      // 比父节点大，则不需要调整了
      if (compare(parent, item) <= 0)
        break;
      // 否则比父节点小，即更快到期，移动父节点往下沉，父节点的位置可用
      heap[pos] = parent;
      // 更新节的位置信息
      if (setPosition !== undefined)
        setPosition(parent, pos);
      // 再往上层比较，或0为了取整
      pos = pos / 2 | 0;
    }
    // pos为item合适的位置，直接赋值
    heap[pos] = item;
    if (setPosition !== undefined)
      setPosition(item, pos);
  }
  // 删除pos索引对应的元素
  removeAt(pos) {
    const heap = this[kHeap];
    // 元素少了一个
    const size = --this[kSize];
    
    // 把最后一个元素补上来成为该子树的根节点，然后开始调整
    heap[pos] = heap[size + 1];
    
    // 删除最后一个元素，即刚才补上去的那个
    heap[size + 1] = undefined;
    
    // 还有元素并且不是最后一个，即被删除的不是倒数第二个元素（倒数第二个叶子）
    if (size > 0 && pos <= size) {
      /*
        二叉堆只保证父子节点的大小关系，不保证左右孩子的大小关系，不像二叉搜索树,
        所以某一个子树的叶子节点可能会比另一个子树的根大

        如果不是根节点并且比父节点小，说明比父节点为根节点的子树所有节点都小，则往上冒，
        如果是根节点则直接往下沉调整
        如果不是根节点但是比父节点大，也有可能比父节点为根的子树中剩下的节点大，所以往下沉调整
      */
      if (pos > 1 && this[kCompare](heap[pos / 2 | 0], heap[pos]) > 0) {
        this.percolateUp(pos);
      } else {
        this.percolateDown(pos);
      }
    }
  }
  // 删除某个值对于的节点
  remove(value) {
    const heap = this[kHeap];
    // 找到位置，然后删除
    const pos = heap.indexOf(value);
    if (pos < 1)
      return false;

    this.removeAt(pos);

    return true;
  }
  // 删除根节点，重新调整二叉堆
  shift() {
    const heap = this[kHeap];
    const value = heap[1];
    if (value === undefined)
      return;

    this.removeAt(1);

    return value;
  }
};

// nodejs的优先队列是基于二叉堆（小根堆）实现的。
// 用数组的方式实现二叉堆。主要的操作包括插入，删除。
// 堆一直保证最小的值是根节点。
// 他是一棵完全二叉树，他的父子节点是父节点的值小于子节点的值，但是不保证兄弟节点间的关系
// 1、插入
// 二叉堆的插入是首先在树的最后位置（对于数组来说就是最后一个元素）插入新增的节点，然后该新节点一直和父节点比较，小于父节点的话就交换，直到大于父节点。这样可以保证根一直是最小值
// 2、删除
// 删除操作首先把需要删除的节点直接删掉，然后把堆的最后一个节点补到被删除节点的位置如果大于父节点，则往下比较。否则往上比较（删除的是根节点的话直接往下比较）。同样，这样的调整保证了根节点是最小值
// 3、往上冒
// 往上冒比较简单，只需要比较父子节点的值，子节点小的话直接往上冒。因为父节点比左右孩子都小，如果当前比较的节点比父小，说明他也比亲兄弟节点小。父子交换可以满足二叉堆的特性
// 4、往下沉
// 往下沉比往上冒父复杂点，他首先要找出孩子中的最小值，然后才能进行比较，交换。见代码注释
