default:
	g++ -O3 -march=native frontend.cpp -o frontend `pkg-config --cflags --libs libdpdk`

run:
	@./frontend -l 3-4 \
      --file-prefix=virtio_frontend \
      --no-telemetry \
      --vdev="net_virtio_user0,path=/tmp/vhost-user.sock,server=0,queues=8,mac=00:11:22:33:44:54"

clean:
	@rm -f backend frontend


      #--vdev="net_virtio_user0,path=/tmp/vhost-user.sock,server=0,queues=2,mac=00:11:22:33:44:54" \
      #--vdev="net_virtio_user1,path=/tmp/vhost-user.sock,server=0,queues=4,mac=00:11:22:33:44:56" \
      #--vdev="net_virtio_user2,path=/tmp/vhost-user.sock,server=0,queues=6,mac=00:11:22:33:44:58" \
      #--vdev="net_virtio_user3,path=/tmp/vhost-user.sock,server=0,queues=8,mac=00:11:22:33:44:60"
