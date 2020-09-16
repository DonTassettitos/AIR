-- NQ5 : finding new auctions with the most number of bids (every 2 seconds, analyzing the last 10 second if possible)
-- using window duration of 10s, slide of 2s

-- ORIGINAL QUERY :

-- SELECT Rstream(auction)
-- FROM (SELECT B1.auction, count(*) AS num
--       FROM Bid [RANGE 60 MINUTE SLIDE 1 MINUTE] B1
--       GROUP BY B1.auction)
-- WHERE num >= ALL (SELECT count(*)
--                   FROM Bid [RANGE 60 MINUTE SLIDE 1 MINUTE] B2
-- GROUP BY B2.auction);

drop table if exists bids cascade;
create table bids (
    bidder_id integer,
    bid_amount integer,
    auction_id integer,
    event_time double precision,
    primary key (bidder_id, auction_id, bid_amount)
);
copy bids from 'DIR/BENCHMARK_NAME/generator-bids.tsv';
alter table bids add column slice_id integer;
update bids set slice_id = floor(event_time / 2);
delete from bids where slice_id < (select min(window_id) from results);
-- update bids set slice_id = slice_id - (select floor(min(slice_id)) from bids);

drop table if exists results cascade;
create table results (
    window_id integer,
    auction_id integer,
    nof_bids integer,
    primary key (window_id, auction_id)
);
copy results from 'DIR/BENCHMARK_NAME/collector.tsv';
-- update results set window_id = window_id - (select min(window_id) from results);

create view slices as 
select slice_id as id,
    auction_id, 
    count(*) as nof_bids
from bids 
group by slice_id, 
    auction_id 
order by slice_id asc, 
    auction_id asc;

create view windows_view as 
select sa.id as window_id,
    sa.auction_id as auction_id, 
    sum(sb.nof_bids) as nof_bids
from slices sa join slices sb 
    on sa.auction_id = sb.auction_id 
    and sa.id <= sb.id 
    and sb.id < sa.id + (10 / 2) 
group by sa.id, sa.auction_id 
order by sa.id, sa.auction_id;

create view sql_results as 
select vb.window_id, 
    vb.auction_id, 
    vb.nof_bids 
from (
        select window_id, 
            max(nof_bids) as nof_bids 
        from windows_view 
        group by window_id
    ) va join windows_view vb 
    on va.window_id = vb.window_id 
    and vb.nof_bids = va.nof_bids
where vb.window_id <= (select max(window_id) from results);

create view comparison as 
select sql_results.window_id as sql_window_id, 
    results.window_id as window_id, 
    sql_results.auction_id as sql_auction_id, 
    results.auction_id as auction_id, 
    sql_results.nof_bids as sql_nof_bids, 
    results.nof_bids as nof_bids 
from sql_results full join results 
    on sql_results.window_id = results.window_id 
    and sql_results.auction_id = results.auction_id 
    and sql_results.nof_bids = results.nof_bids
order by sql_results.window_id, 
    results.window_id;

-- when i delete only once it doesn't seem to work, i don't know why
delete from bids where slice_id < (select min(window_id) from results);