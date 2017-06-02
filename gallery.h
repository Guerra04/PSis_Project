//TODO mudar o nome para API.h
#ifndef GALLERY_LIB
#define GALLERY_LIB

/*******************************************************************************
	Establishes an UDP connection with the gateway and requests the ip
address and port of the TCP socket of one peer. Then it establishes a TCP
connection with said peer and closes the communication with the gateway

Returns:
	- >0: file descriptor of the socket of the TCP connection with the peer
	- 0: no peer is available
	- -1: communication error
*******************************************************************************/
int gallery_connect(char * host, in_port_t port);

/*******************************************************************************
	Sends the name and the size of the photo to the peer, receives the id of the
photo and then sends the photo in a byte stream.

Returns:
	- >0: id of the photo that was inserted in the gallery
	- 0: error inserting photo
*******************************************************************************/
uint32_t gallery_add_photo(int peer_socket, char *file_name);

/*******************************************************************************
	Sends id of the photo in which the client wants to insert a keyword and the
keyword to be inserted. Receives an integer to check if the operation was
succesfull or not.

Returns:
	- 1: success
	- 0: communication error
	- -1: keyword list full
	- -2: photo doesn't exist
	- -3: keyword already exists
*******************************************************************************/
int gallery_add_keyword(int peer_socket, uint32_t id_photo, char *keyword);

/*******************************************************************************
	Sends a keyword to the peer. Receives the number of photos that contain the
sent keyword and then receives an array with the ids of those photos and stores
them in 'id_photos'.

Returns:
	- >0: length of photo list with argument keyword
	- 0: no photo with argument keyword
	- -1: communication error
*******************************************************************************/
int gallery_search_photo(int peer_socket, char * keyword, uint32_t ** id_photos);

/*******************************************************************************
	Sends id of the photo to be deleted. Receives an integer to check if the
operation was succesfull or not.

Returns:
	- 1: Photo was deleted
	- 0: Photo with the sent id doesn't exist in the gallery
*******************************************************************************/
int gallery_delete_photo(int peer_socket, uint32_t id_photo);

/*******************************************************************************
	Sends id of the photo to be searched. Receives the length of the name of the
photo and then if that length > 0 (photo exists) receives the name of the photo
and stores it in 'photo_name'.

Returns:
	- 1: photo exists and name was retrieved
	- 0: photo doesn't exists
	- -1: communication error
*******************************************************************************/
int gallery_get_photo_name(int peer_socket, uint32_t id_photo, char **photo_name);

/*******************************************************************************
	Sends an id of a photo to the peer. Receives the size of the photo to be
received and then if that size > 0 receives the photo in a byte stream and
writes it to a file with name 'file_name', storing it in the disk.

Returns:
	- 1: photo downloaded succesfully
	- 0: photo does not exist
	- -1: communication error
*******************************************************************************/
int gallery_get_photo(int peer_socket, uint32_t id_photo, char *file_name);

#endif //GALLERY_LIB
