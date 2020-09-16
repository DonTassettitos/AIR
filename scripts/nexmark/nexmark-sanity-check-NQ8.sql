-- NQ8 : finding new auctions created by new users (every 10 seconds for both)
-- using window duration of 10s 

-- ORIGINAL QUERY :

-- SELECT Rstream(auction)
-- FROM (SELECT B1.auction, count(*) AS num
--       FROM Bid [RANGE 60 MINUTE SLIDE 1 MINUTE] B1
--       GROUP BY B1.auction)
-- WHERE num >= ALL (SELECT count(*)
--                   FROM Bid [RANGE 60 MINUTE SLIDE 1 MINUTE] B2
-- GROUP BY B2.auction);

drop table if exists persons cascade;
drop table if exists auctions cascade;

create table persons (
    id integer, 
    full_name varchar(35),
    event_time double precision,
    primary key (id)
);

create table auctions (
    id integer,
    seller_id integer,
    reserve_price integer,
    event_time double precision,
    primary key (id)
);

copy persons from 'DIR/BENCHMARK_NAME/generator-persons.tsv';
copy auctions from 'DIR/BENCHMARK_NAME/generator-auctions.tsv';

drop table results cascade;

create table results (
    window_id integer,
    person_id integer,
    seller_name varchar(35),
    auction_id integer,
    reserve_price integer,
    primary key (person_id, auction_id)
);

copy results from 'DIR/BENCHMARK_NAME/collector.tsv';

alter table auctions add window_id integer;
update auctions set event_time = event_time - (select floor(min(event_time)) from auctions);
update auctions set window_id = floor(event_time / 10);
update auctions set window_id = window_id - (select min(window_id) from auctions);

alter table persons add window_id integer;
update persons set event_time = event_time - (select floor(min(event_time)) from persons);
update persons set window_id = floor(event_time / 10);
update persons set window_id = window_id - (select min(window_id) from persons);

create table temp (a integer);
insert into temp (a) select max(window_id) from results;
insert into temp (a) select max(window_id) from auctions;
insert into temp (a) select max(window_id) from persons;

delete from auctions where window_id > (select min(a) from temp);
delete from persons where window_id > (select min(a) from temp);
delete from results where window_id > (select min(a) from temp);
drop table temp;

create view sql_results as (
select persons.window_id as window_id, 
        persons.id as person_id, 
        persons.full_name as seller_name, 
        auctions.id as auction_id, 
        auctions.reserve_price as reserve_prices 
from persons join auctions 
    on persons.window_id = auctions.window_id 
    and persons.id = auctions.seller_id
);

create view comparison as (
select sql_results.window_id as sql_window_id, 
    results.window_id as window_id, 
    sql_results.auction_id as sql_auction_id, 
    results.auction_id as auction_id, 
    sql_results.person_id as sql_person_id, 
    results.person_id as person_id 
from sql_results full join results 
    on sql_results.window_id = results.window_id 
    and sql_results.auction_id = results.auction_id 
    and sql_results.person_id = results.person_id 
order by sql_results.window_id asc, 
    sql_results.auction_id asc
);