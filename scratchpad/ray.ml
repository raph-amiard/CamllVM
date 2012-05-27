(* ocamlc -rectypes scratchpad/ray.ml *)

let delta = sqrt epsilon_float

type vec = {x:float; y:float; z:float}
let zero = {x=0.; y=0.; z=0.}
let ( *| ) s r = {x = s *. r.x; y = s *. r.y; z = s *. r.z}
let ( +| ) a b = {x = a.x +. b.x; y = a.y +. b.y; z = a.z +. b.z}
let ( -| ) a b = {x = a.x -. b.x; y = a.y -. b.y; z = a.z -. b.z}
let dot a b = a.x *. b.x +. a.y *. b.y +. a.z *. b.z
let length r = sqrt(dot r r)
let unitise r = 1. /. length r *| r

let ray_sphere orig dir center radius =
  let v = center -| orig in
  let b = dot v dir in
  let d2 = (b *. b -. dot v v +. radius *. radius) in
  if d2 < 0. then infinity else
  let d = sqrt d2 in
  let t1 = b -. d and t2 = b +. d in
  if t2>0. then if t1>0. then t1 else t2 else infinity

type uu = vec * float * uu list;;

let rec intersect orig dir (l, _ as hit) ((center, radius, scene): uu) =
  match ray_sphere orig dir center radius, scene with
  | l', _ when l' >= l -> hit
  | l', [] -> l', unitise (orig +| l' *| dir -| center)
  | _, _ -> intersects orig dir hit scene
and intersects orig dir hit z = match (z:uu list) with (*function*)
  | [] -> hit
  | scene::scenes -> intersects orig dir (intersect orig dir hit scene) scenes

let light = unitise {x=1.; y=3.; z= -2.} and ss = 4

let rec ray_trace dir scene =
  let l, n = intersect zero dir (infinity, zero) scene in
  let g = dot n light in
  if g <= 0. then 0. else
    let p = l *| dir +| sqrt epsilon_float *| n in
    if fst (intersect p light (infinity, zero) scene) < infinity then 0. else g

let rec create level c r =
  let obj = c, r, [] in
  if level = 1 then obj else
    let a = 3. *. r /. sqrt 12. in
    let aux x' z' = create (level - 1) (c +| {x=x'; y=a; z=z'}) (0.5 *. r) in
    c, 3. *. r, [obj; aux (-.a) (-.a); aux a (-.a); aux (-.a) a; aux a a]

let level, n =
  try int_of_string Sys.argv.(1), int_of_string Sys.argv.(2) with _ -> 3, 100 (*6, 512*)

let scene = create level {x=0.; y= -1.; z=4.} 1.;;

let () =
  Printf.printf "P5\n%d %d\n255\n" n n;
  for y = n - 1 downto 0 do
    for x = 0 to n - 1 do
      let g = ref 0. in
      for dx = 0 to ss - 1 do
	for dy = 0 to ss - 1 do
	  let aux x d = float x -. float n /. 2. +. float d /. float ss in
	  let dir = unitise {x=aux x dx; y=aux y dy; z=float n} in
	  g := !g +. ray_trace dir scene
	done;
      done;
      let g = 0.5 +. 255. *. !g /. float (ss*ss) in
      Printf.printf "%c" (char_of_int (int_of_float g))
    done;
  done;
  ()
;;

(*
let () =
  for i = 1 to 2 do
    Printf.eprintf "i=%d\n" i;
    let scene = create level {x=0.; y= -1.; z=4.} 1. in
    for y = n - 1 downto 0 do
      for x = 0 to n - 1 do
	let g = ref 0. in
	for dx = 0 to ss - 1 do
	  for dy = 0 to ss - 1 do
	    let aux x d = float x -. float n /. 2. +. float d /. float ss in
	    let dir = unitise {x=aux x dx; y=aux y dy; z=float n} in
	    g := !g +. ray_trace dir scene
	  done;
	done;
	let _g = 0.5 +. 255. *. !g /. float (ss*ss) in
	()
      done;
    done;
  done;
  ()
;;
*)
