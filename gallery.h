#ifndef GALLERY_LIB
#define GALLERY_LIB

int gallery_connect(char * host, uint32_t port);

uint32_t gallery_add_photo(int peer_socket, char *file_name);

int gallery_add_keyword(int peer_socket, uint32_t id_photo, char *keyword);

int gallery_search_photo(int peer_socket, char * keyword, uint32_t ** id_photos);

int gallery_delete_photo(int peer_socket, uint32_t id_photo);

int gallery_get_photo_name(int peer_socket, uint32_t id_photo, char **photo_name);

int gallery_get_photo(int peer_socket, uint32_t id_photo, char *file_name);

#endif //GALLERY_LIB
