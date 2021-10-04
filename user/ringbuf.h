

int create_ring_buffer(char *name);

int delete_ring_buffer(int ring_buffer_descriptor);

void store(int *p, int v);

int load(int *p);

void ringbuf_start_read(int ring_desc, char **addr, int *bytes);
void ringbuf_finish_read(int ring_desc, int bytes);

void ringbuf_start_write(int ring_desc, char **addr, int *bytes);
void ringbuf_finish_write(int ring_desc, int bytes);
