CFLAGS := -Wall -Werror -I../include -pthread

all: approximate_counter test_linked_list test_queue test_hash_table simple_concurrent_counter \
		hw_test_simple_concurrent_counter hw_test_approximate_counter hw_test_hand-over-hand-locking-list \
		btree test_hashmap

clean:
	rm -f approximate_counter test_linked_list test_queue test_hash_table simple_concurrent_counter \
		hw_test_simple_concurrent_counter hw_test_approximate_counter hw_test_hand-over-hand-locking-list \
		btree test_hashmap

approximate_counter:
	gcc $(CFLAGS) -o approximate_counter approximate_counter.c

test_linked_list:
	gcc $(CFLAGS) -o test_linked_list test_linked_list.c concurrent_linked_list.c

test_queue:
	gcc $(CFLAGS) -o test_queue test_queue.c concurrent_queue.c

test_hash_table:
	gcc $(CFLAGS) -o test_hash_table test_hash_table.c concurrent_hash_table.c concurrent_linked_list.c

simple_concurrent_counter:
	gcc $(CFLAGS) -o simple_concurrent_counter simple_concurrent_counter.c

hw_test_simple_concurrent_counter:
	gcc $(CFLAGS) -o hw_test_simple_concurrent_counter hw_test_simple_concurrent_counter.c

hw_test_approximate_counter:
	gcc $(CFLAGS) -o hw_test_approximate_counter hw_test_approximate_counter.c

hw_test_hand-over-hand-locking-list:
	gcc $(CFLAGS) -o hw_test_hand-over-hand-locking-list hw_test_hand-over-hand-locking-list.c hand-over-hand-locking-list.c

btree:
	gcc $(CFLAGS) -o btree btree.c

test_hashmap:
	gcc $(CFLAGS) -o test_hashmap test_hashmap.c concurrent_hashmap.c