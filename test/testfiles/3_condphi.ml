let fn a b =
    let c = 12 in
    let d = 15 in
    let e = if a > 15 then a + c else a + d in
    if b > 12 then e else 42;;

fn 6 7;;
