Şimdi ilk olarak biz bir serverı posix sys callar ile dinliyoruz -> int server_fd = socket(AF_INET, SOCK_STREAM, 0); -> Burada server_fd bir file descriptor(1,5 10 gibi bir int). 

Sonrasında öncelikle bir sockaddr_in türünde bir değişken oluşturup bunu IP_4 türünde olan ve portu Redisin kendi orjinal portu olan 6379 a set bind ediyoruz.


1-> RESP(Redis Serialization Protocol) -> Redisin kendi metinsel iletişim protokolü. Biz de ilk olarak client'tan gelen mesajı önemsemeden her gelen istekte +PONG\r\n response'unu dönüyoruz.

2-> Şimdi aynı anda hem yeni client hem de bir clienttan birden fazla message alabilmek adına bir event loop kodladım. Burada select çağrısını kullanıyoruz posixten. Select olana kadar kod uykuya dalıyor fakat select çağrıldıktan sonrasında kod 
