# Physics Model: Sources

[← Overview](Physics-Model-Overview) · [Core Model](Physics-Model-Core) · [Second-Tier Model](Physics-Model-Second-Tier)

[[_TOC_]]

## References

1. Milliken, W. F., & Milliken, D. L. (1995). *Race Car Vehicle Dynamics*. SAE International.
2. Katz, J. (2006). Aerodynamics of race cars. *Annual Review of Fluid Mechanics*, 38. (Not independently confirmed.) Also: Katz, J. (1995). *Race Car Aerodynamics: Designing for Speed*. Bentley Publishers (ISBN 978-0-8376-0142-7; later printings dated 1996).
3. Zhang, X., Toet, W., & Zerihan, J. (2006). Ground effect aerodynamics of race cars. *Applied Mechanics Reviews*, 59(1), DOI 10.1115/1.2110263. Page range reported inconsistently across sources (33–48 vs 33–49); omit pages.
4. Anderson, J. D. *Fundamentals of Aerodynamics*. McGraw-Hill.
5. Pacejka, H. B. (2012). *Tire and Vehicle Dynamics* (3rd ed.). Butterworth-Heinemann, ISBN 978-0-08-097016-5.
6. Gillespie, T. D. (1992). *Fundamentals of Vehicle Dynamics*. SAE International (R-114), ISBN 1-56091-199-9.
7. Brayshaw, D. L., & Harrison, M. F. (2005). A quasi steady state approach to race car lap simulation in order to understand the effects of racing line and centre of gravity location. *Proc. IMechE Part D: Journal of Automobile Engineering*, 219(6). **Related work only:** it is an optimal-control-style QSS method with a 7-DOF model, not the three-pass scheme used here.
8. Kelly, D. P. (2008). *Lap time simulation with transient vehicle and tyre dynamics*. PhD thesis, Cranfield University.
9. Chalkiopoulos, M. *OpenLAP: Lap Time Simulator* (GitHub: mc12027/OpenLAP-Lap-Time-Simulator, GPL v3). (Author name corrected from an earlier draft.)
10. Heywood, J. B. (2018). *Internal Combustion Engine Fundamentals* (2nd ed.). McGraw-Hill, ISBN 978-1-260-11610-6.
11. Incropera, F. P., DeWitt, D. P., Bergman, T. L., & Lavine, A. S. *Fundamentals of Heat and Mass Transfer*. Wiley.
12. Çengel, Y. A., & Boles, M. A. *Thermodynamics: An Engineering Approach*. McGraw-Hill.
13. Limpert, R. (2011). *Brake Design and Safety* (3rd ed.). SAE International (R-398).
14. Farroni, F., Giordano, D., Russo, M., & Timpone, F. (2014). TRT: Thermo Racing Tyre, a physical model to predict the tyre temperature distribution. *Meccanica*, 49(3), 707–723.
15. Heilmeier, A., Graf, M., & Lienkamp, M. (2018). A race simulation for strategy decisions in circuit motorsports. *IEEE International Conference on Intelligent Transportation Systems (ITSC)*. (Title confirmed via TUM mediaTUM; author list and pages not confirmed. Also relevant: Heilmeier et al. (2020), *Applied Sciences* 10(12):4229, and the 2022 TUM dissertation.)
16. Horne, W. B., & Dreher, R. C. (1963). *Phenomena of pneumatic tire hydroplaning*. NASA TN D-2056.
17. FIA. *2026 Formula 1 Regulations, Section C: Technical*, Issue 20 (2026-08-05), Articles C4.1, C4.3, C4.5. https://www.fia.com (the primary source for mass; article numbers and values change between issues)
18. OpenF1 API documentation. https://openf1.org/docs (historical data from 2023 free without authentication; car_data about 3.7 Hz; real-time requires a paid plan).
19. Picard, A., Davis, R. S., Gläser, M., & Fujii, K. (2008). Revised formula for the density of moist air (CIPM-2007). *Metrologia*, 45.
20. Alduchov, O. A., & Eskridge, R. E. (1996). Improved Magnus form approximation of saturation vapor pressure. *Journal of Applied Meteorology*, 35.
21. Red Bull Racing. *Bull's guide to the F1 2026 regulations: technical regulations* (quotes 768 kg; **superseded** by FIA Issue 20, see [17]). https://www.redbullracing.com/int-en/projects/bulls-guide-to-the-f1-2026-regulations/technical-regulations (secondary source)
22. Wong, J. Y. *Theory of Ground Vehicles*. Wiley.
23. Speedcafe. *Weight increase for 2025 F1 cars* (quotes 768 kg and an 80 kg driver; **superseded** by FIA Issue 20, see [17]). https://speedcafe.com/weight-increase-for-2025-f1-cars/ (secondary source)
24. Rossi, et al. (2020). A simple mono-dimensional approach for lap time optimisation. *Applied Sciences*, 10(4), 1498. https://www.mdpi.com/2076-3417/10/4/1498 (describes QSS forward/backward integration from apexes; first author spelling from the repository filename).
25. *Minimum-lap-time optimization and simulation* (survey), *Vehicle System Dynamics* (2021). Authors not confirmed.
26. Horne, W. B. (1975). *Wet runways*. NASA TM X-72650. (Source of the dynamic hydroplaning speed rule of thumb, about 9 times the square root of tire pressure in psi, speed in knots; the Horne and Dreher 1963 form is about 10.35 times the square root of pressure in psi, speed in mph.)
27. Milliken & Milliken load-sensitivity data is quoted in the Wikipedia article *Tire load sensitivity* (p. 27 of [1]); Wright, P. (2001). *Formula 1 Technology*. SAE. (secondary)


## Citation verification status

Verification pass: 16 Nov 2026. "Confirmed" means a primary or authoritative page was retrieved and agrees. "Partly" means the claim is supported only by secondary pages (course notes, encyclopedias, quoting sites). "Not found" means I could not open the source and the claim rests on convention or memory. Formulas that are our own design are labelled engineering assumptions regardless of source status.

| # | Used for | Status | Notes |
|---|---|---|---|
| [1] | C1, C3, C4, C7, C8, T4 | Partly | Book details confirmed (SAE R-146, ISBN 1-56091-526-9). Load sensitivity (coefficient falls as load rises) confirmed via secondary summaries of p. 27 and Fig. 2.9; the book itself not opened. |
| [2] | C2, T5 | Partly | Book confirmed (Bentley, ISBN 0-8376-0142-8). Annual Review article not independently confirmed. Specific ground-effect behaviors not checked page by page. |
| [3] | C2, T5 | Confirmed (existence) | DOI and authors confirmed; page range inconsistent between sources. Content not read. |
| [4] | C2 | Not found | Textbook; finite-wing lift slope a0/(1+a0/(πAR)) and induced drag form confirmed via MIT 16.01 course notes instead. |
| [5] | C4 | Confirmed | Edition and ISBN confirmed. Magic Formula y = D sin(C arctan(Bx − E(Bx − arctan Bx))) confirmed in several papers and TNO slides. Suggested C ≈ 1.65 longitudinal, 1.3 lateral as in the cited paper. |
| [6] | C1, C3, C5, C6, C7, T4 | Partly | Book details confirmed. Understeer gradient K = Wf/Cαf − Wr/Cαr (rad/g, weights in N) confirmed in course notes; the 1/g factor in the earlier draft was wrong and has been removed. Effective mass and C_rr = C0 + C2v² forms not confirmed from Gillespie; the confirmed rolling-resistance forms found online differ (e.g. 0.005 + (1/p)(0.01 + 0.0095(v/100)²)), so C0 and C2 stay **[CAL]**. |
| [7] | C8 | Corrected | Not the three-pass scheme; optimal-control-style QSS with a 7-DOF model. Now cited as related work only. |
| [8] | C4, C8, T1 | Confirmed | Kelly Cranfield thesis found. |
| [9] | C8 | Confirmed | Author is Chalkiopoulos (corrected); repo is GPL v3, so reading it for ideas is fine but copying code would affect our license. |
| [10] | C5, T6, T7 | Partly | Book and ISBN confirmed. Power proportional to air density confirmed in a NACA report; with temperature the dependence is weaker than 1/T (volumetric efficiency rises with temperature), so avoid a pure ρ-proportional derate without a **[CAL]** exponent. |
| [11] | T1, T3, T7 | Partly | Lumped capacitance valid for Biot number below about 0.1 confirmed via course notes; Incropera not opened. |
| [12] | T1 | Not found | Ideal-gas law, textbook standard; not checked against the book. |
| [13] | C7, T3 | Partly | Book details confirmed. Ideal brake force distribution (match brake split to dynamic axle loads) confirmed via brake-design course notes; Limpert not opened. |
| [14] | T1, T2 | Corrected | Authors corrected to Farroni, Giordano, Russo, Timpone; Meccanica 49(3):707–723. Content not read. |
| [15] | T2, T8 | Partly | Title confirmed; author list and pages not confirmed; fetch of the MDPI 2020 paper was blocked so the degradation form (linear vs cliff) and the fuel-mass sensitivity are **not verified**. |
| [16] | T8 | Confirmed | NASA TN D-2056 found. Rule of thumb 9√p (knots) from Horne is repeated by NASA-derived pilot material; the 10.35√p (mph) form is the older NASA equation. |
| [17] | C1, C2, C5, T6 | Confirmed (mass) | FIA Issue 20 gives 724 kg + nominal tyre mass (726 in Qualifying), 82 kg driver reference mass. Wheelbase 3400 mm (C2.3.3). Fuel and energy limit values (C5, C16) were not in the retrieved portion. A forum post says 2026 moves to a 3000 MJ/h energy-flow limit; unverified. |
| [18] | Overview: validation plan | Confirmed | Docs state free historical data from 2023 without authentication. |
| [19] | C2, T7 | Confirmed (existence) | CIPM-2007 paper located. |
| [20] | C2, T7 | Confirmed | Magnus constants 6.1094, 17.625, 243.04 confirmed. |
| [21] | C1 | Superseded | Secondary; older mass figures. |
| [22] | C6 | Partly | Table of contents confirms a rolling-resistance section (1.2) in the 5th edition; content not opened. |
| [23] | C1 | Superseded | Secondary; older mass figures. |
| [24] | C8 | Confirmed | Describes QSS apex plus forward/backward integration. |
| [25] | C8 | Partly | Authors not confirmed. |
| [26] | T8 | Confirmed | NASA TM X-72650 found. |
| ISA / barometric formula | C2, T7 | Confirmed | p = p0(1 − 0.0065h/288.15)^5.25588. |
| F1 mass sensitivity | T6 | Partly | Press estimates range from about 0.03 s per kg to about 0.035 s per kg per lap (roughly 0.3 s per 10 kg); no primary source. Keep as a validation target. |
| F1 thermal efficiency | T6 | Partly | Mercedes reported above 50% on the dyno (2017 press); 2026 engines unverified. |
| Self-designed models | C2 floor scheme, T1 Gaussian temperature grip, T2 wear, T8 surface evolution | Engineering assumption | Not taken from a source; calibrate against OpenF1 data. |
