'use strict';

const {
  Array,
} = primordials;


const kSize = 2048;
const kMask = kSize - 1;

class FixedCircularBuffer {
  constructor () {
    this.bottom = 0;
    this.top = 0;
    this.list = new Array(kSize);
    this.next = null;
  }

  isEmpty () {
    return this.top === this.bottom;
  }

  // 要判断回环
  isFull () {
    return ((this.top + 1) & kMask) === this.bottom;
  }

  push (data) {
    this.list[this.top] = data;
    this.top = (this.top + 1) & kMask;
  }

  // 移除一个元素，更新位置
  shift () {
    const nextItem = this.list[this.bottom];

    // 没有元素了，不需要更新位置
    if (nextItem === undefined) {
      return null;
    }

    this.list[this.bottom] = undefined;
    this.bottom = (this.bottom + 1) & kMask;
    return nextItem;
  }
};

class FixedQueue {
  constructor () {
    this.head = this.tail = new FixedCircularBuffer();
  }

  isEmpty () {
    return this.head.isEmpty();
  }

  push (data) {
    // 满了则申请一个新的，head指向新的，tail指向最开始的那个，即最旧的
    if (this.head.isFull()) {
      // Head is full: Creates a new queue, sets the old queue's `.next` to it,
      // and sets it as the new main queue.
      this.head = this.head.next = new FixedCircularBuffer();
    }

    this.head.push(data);
  }

  shift () {
    const tail = this.tail;
    const next = tail.shift();

    // 消费完一个FixedCircularBuffer了，下一个
    if (tail.isEmpty() && tail.next !== null) {
      // If there is another queue, it forms the new tail.
      this.tail = tail.next;
    }

    return next;
  }
};