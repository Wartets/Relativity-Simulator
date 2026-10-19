# spsc_queue

Verifies the lock-free single-producer single-consumer ring buffer: capacity, ordering, wrap-around and an integrity check over half a million elements transferred between two threads.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 12 |
| Failed checks | 0 |
| Duration | 0.1296 s |
| Generated | 2026-10-19 22:21:18 UTC |

## Sequential behaviour

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a new queue is empty |  |
| PASS | the capacity equals the template parameter |  |
| PASS | fifteen elements fit in a queue of capacity sixteen |  |
| PASS | the size reflects the pushed elements |  |
| PASS | elements are popped in first-in first-out order |  |
| PASS | the queue is empty after popping every element |  |
| PASS | popping an empty queue fails |  |
| PASS | repeated fill and drain cycles preserve order across index wrap-around |  |

## Concurrent producer and consumer

| Result | Check | Detail |
| --- | --- | --- |
| PASS | no element is lost, duplicated or reordered across threads |  |
| PASS | the consumer receives every element |  |
| PASS | the sum of consumed elements matches the arithmetic series |  |
| PASS | the queue is empty after the transfer |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Concurrent producer and consumer | transferred_elements | 500000 | elements |

