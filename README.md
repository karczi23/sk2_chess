Karol Koźlinka 155929

# Sprawozdanie SK2


## Opis projektu
### Gra w szachy
Projekt implementuje serwer oraz klienta do obsługi gry w szachy.

Użyte języki: 

Serwer: C 

Klient: Python

## Opis komunikacji pomiędzy serwerem i klientem 
Komunikacja odbywa się za pomocą socketów TCP z wykorzystaniem `send()` i `read()`. Protokół ten w tym przypadku sprawdza się lepiej, ponieważ przy grze w szachy wymagana jest pełna zgodność przesyłanych danych i nawet jednorazowo niedostarczone poprawnie dane mogą mieć tragiczne skutki dla przebiegu gry. Walidacja poprawności ruchu odbywa się po stronie serwera. Protokół używa prostych poleceń tekstowych:

Wiadomości od serwera do klienta:

`board <STAN PLANSZY>` - zawiera aktualny stan planszy

`e <BŁĄD>` -  zawiera komunikat błędu np. o nieprawidłowym ruchu

`x <WIADOMOŚĆ>` - komunikat o zakończeniu gry

Wiadomości od klienta do serwera:

ruch na planszy np `e2e3` - pierwsze dwa znaki to pole figury, która ma wykonać ruch, natomiast dwa ostatnie znaki to pole, na które figura ma się przemieścić

Klient z serwerem wymieniają wiadomości naprzemiennie. W zależności od typu wiadomości (typy w punkcie wyżej) wykonywane są różne akcje. Serwer korzysta z funkcji `select`, aby obsłużyć poszczególnych klientów. Zapewnia to możliwość prowadzenia wiele rozgrywek naraz. Serwer zarządza komunikacją wysyłając odpowiednie wiadomości do graczy. Oczekuje na ich informacje zwrotne oraz informuje ich o aktualnym przebiegu gry.

## Opis plików źródłowych:

`main.c` - obsługa sieciowego aspektu programu, obsługa gier

`utils.c` - funkcje pomocnicze do sprawdzania poprawności przebiegu gry

`client.py` - klient pozwalający na prowadzenie gry

## Uruchamianie serwera i klienta:

serwer:

`make clean`

`make`

`make run`

debug:

`make clean`

`make debug`

`make gdb`

klient:

`python3 client.py`
