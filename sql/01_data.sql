INSERT INTO questions (question) VALUES ('What is the diameter of the moon?');

INSERT INTO answers (qid, answer) VALUES
    ((SELECT max(qid) FROM questions), '3474800 m'),
    ((SELECT max(qid) FROM questions), '3474.8 km'),
    ((SELECT max(qid) FROM questions), '2159.1 miles');

INSERT INTO keywords (qid, keyword) VALUES
    ((SELECT max(qid) FROM questions), 'diameter'),
    ((SELECT max(qid) FROM questions), 'moon');


/* ----- new question ----- */

INSERT INTO questions (question) VALUES ('What is the temperature of the Sun?');

INSERT INTO answers (qid, answer) VALUES
    ((SELECT max(qid) FROM questions), '5,778 K'),
    ((SELECT max(qid) FROM questions), '5504.85 C');

INSERT INTO keywords (qid, keyword) VALUES
    ((SELECT max(qid) FROM questions), 'temperature'),
    ((SELECT max(qid) FROM questions), 'sun');

/* ----- new question ----- */


INSERT INTO questions (question) VALUES ('What is the mass of an electron?');

INSERT INTO answers (qid, answer) VALUES
    ((SELECT max(qid) FROM questions), '9.1093837E-31 kg'),
    ((SELECT max(qid) FROM questions), '0.00091093837E-27 kg'),
    ((SELECT max(qid) FROM questions), '0.00000000000091093837E-18 kg'),
    ((SELECT max(qid) FROM questions), '0.0000000000000000000000000000091093837 kg');

INSERT INTO keywords (qid, keyword) VALUES
    ((SELECT max(qid) FROM questions), 'mass'),
    ((SELECT max(qid) FROM questions), 'electron');


/* ----- new question ----- */


INSERT INTO questions (question) VALUES ('What happens within an isolated container enclosing a gas with an uneven temperature distribution?');


INSERT INTO answers (qid, answer) VALUES
    ((SELECT max(qid) FROM questions), 'Random interactions between molecules change the temperature of the gas over time until it reaches thermal equilibrium.'),
    ((SELECT max(qid) FROM questions), 'The average velocity of each molecule becomes closer together as the molecules tranfer kinetic energy to each other.');

INSERT INTO keywords (qid, keyword) VALUES
    ((SELECT max(qid) FROM questions), 'temperature'),
    ((SELECT max(qid) FROM questions), 'thermal equilibrium');


/* ----- new question ----- */


INSERT INTO questions (question) VALUES ('How fast is plaid speed?');

INSERT INTO answers (qid, answer) VALUES (
    (SELECT max(qid) FROM questions),
'A arbitrarily high magnitude of velocity that, in accordance with Einstein''s general theory of relativity,
must be upper bound by the speed that light travels through a vacuum (299,792,458 m/s).');

INSERT INTO keywords (qid, keyword) VALUES
    ((SELECT max(qid) FROM questions), 'speed'),
    ((SELECT max(qid) FROM questions), 'vacuum'),
    ((SELECT max(qid) FROM questions), 'light'),
    ((SELECT max(qid) FROM questions), 'plaid');